#include "mdv_channel.h"
#include "mdv_messages.h"
#include <mdv_version.h>
#include <mdv_log.h>
#include <mdv_uuid.h>
#include <mdv_dispatcher.h>
#include <mdv_socket.h>
#include <mdv_rollbacker.h>
#include <mdv_condvar.h>
#include <stdatomic.h>


/// User connection context used for storing different type of information
/// about connection
struct mdv_channel
{
    atomic_uint_fast32_t    rc;             ///< references counter
    volatile bool           connected;      ///< connection status
    mdv_condvar             conn_signal;    ///< CV for connection signalization
    mdv_dispatcher         *dispatcher;     ///< Messages dispatcher
    mdv_uuid                uuid;           ///< server uuid
};


/**
 * @brief   Say Hey! to server
 */
static mdv_errno mdv_user_wave(mdv_channel *channel)
{
    mdv_msg_hello const hello =
    {
        .uuid    = {},
        .version = MDV_VERSION
    };

    binn hey;

    if (!mdv_msg_hello_binn(&hello, &hey))
        return MDV_FAILED;

    mdv_msg message =
    {
        .hdr =
        {
            .id = mdv_message_id(hello),
            .size = binn_size(&hey)
        },
        .payload = binn_ptr(&hey)
    };

    mdv_errno err = mdv_channel_post(channel, &message);

    binn_free(&hey);

    return err;
}


static mdv_errno mdv_channel_wave_handler(mdv_msg const *msg, void *arg)
{
    MDV_LOGI("<<<<< '%s'", mdv_msg_name(msg->hdr.id));

    mdv_channel *channel = arg;

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_msg_name(msg->hdr.id));
        return MDV_FAILED;
    }

    mdv_msg_hello client_hello = {};

    if (!mdv_msg_hello_unbinn(&binn_msg, &client_hello))
    {
        MDV_LOGE("Invalid '%s' message", mdv_msg_name(msg->hdr.id));
        binn_free(&binn_msg);
        return MDV_FAILED;
    }

    binn_free(&binn_msg);

    if(client_hello.version != MDV_VERSION)
    {
        MDV_LOGE("Invalid client version");
        return MDV_FAILED;
    }

    channel->uuid = client_hello.uuid;

    channel->connected = true;

    return mdv_condvar_signal(&channel->conn_signal);
}


static mdv_errno mdv_channel_status_handler(mdv_msg const *msg, void *arg)
{
    MDV_LOGI("<<<<< '%s'", mdv_msg_name(msg->hdr.id));
    (void)msg;
    (void)arg;
    return MDV_NO_IMPL;
}


mdv_channel * mdv_channel_create(mdv_descriptor fd)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_channel *channel = mdv_alloc(sizeof(mdv_channel), "channel");

    if (!channel)
    {
        MDV_LOGE("No memory for user connection context");
        mdv_rollback(rollbacker);
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, channel, "channel");

    MDV_LOGD("Channel %p initialization", channel);

    atomic_init(&channel->rc, 1);

    channel->connected = false;

    // Create conditional variable for connection status waiting

    if (mdv_condvar_create(&channel->conn_signal) != MDV_OK)
    {
        MDV_LOGE("Conditional variable not created");
        mdv_rollback(rollbacker);
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_condvar_free, &channel->conn_signal);

    channel->dispatcher = mdv_dispatcher_create(fd);

    if (!channel->dispatcher)
    {
        MDV_LOGE("Messages dispatcher not created");
        mdv_rollback(rollbacker);
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_dispatcher_free, channel->dispatcher);

    mdv_dispatcher_handler const handlers[] =
    {
        { mdv_message_id(hello),    &mdv_channel_wave_handler,   channel },
        { mdv_message_id(status),   &mdv_channel_status_handler, channel }
    };

    for(size_t i = 0; i < sizeof handlers / sizeof *handlers; ++i)
    {
        if (mdv_dispatcher_reg(channel->dispatcher, handlers + i) != MDV_OK)
        {
            MDV_LOGE("Messages dispatcher handler not registered");
            mdv_rollback(rollbacker);
            mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
            return 0;
        }
    }

    if (mdv_user_wave(channel) != MDV_OK)
    {
        MDV_LOGD("User handshake message failed");
        mdv_rollback(rollbacker);
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    MDV_LOGD("Channel %p initialized", channel);

    mdv_rollbacker_free(rollbacker);

    return channel;
}


static void mdv_channel_free(mdv_channel *channel)
{
    mdv_dispatcher_free(channel->dispatcher);
    mdv_condvar_free(&channel->conn_signal);
    mdv_free(channel, "channel");
    MDV_LOGD("Channel %p freed", channel);
}


mdv_channel * mdv_channel_retain(mdv_channel *channel)
{
    atomic_fetch_add_explicit(&channel->rc, 1, memory_order_acquire);
    return channel;
}


uint32_t mdv_channel_release(mdv_channel *channel)
{
    uint32_t rc = 0;

    if (channel)
    {
        rc = atomic_fetch_sub_explicit(&channel->rc, 1, memory_order_release) - 1;
        if (!rc)
            mdv_channel_free(channel);
    }

    return rc;
}


mdv_uuid const * mdv_channel_uuid(mdv_channel *channel)
{
    return &channel->uuid;
}


mdv_errno mdv_channel_send(mdv_channel *channel, mdv_msg *req, mdv_msg *resp, size_t timeout)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(req->hdr.id));

    mdv_errno err = MDV_CLOSED;

    channel = mdv_channel_retain(channel);

    if (channel)
    {
        err = mdv_dispatcher_send(channel->dispatcher, req, resp, timeout);
        mdv_channel_release(channel);
    }

    return err;
}


mdv_errno mdv_channel_post(mdv_channel *channel, mdv_msg *msg)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(msg->hdr.id));

    mdv_errno err = MDV_CLOSED;

    channel = mdv_channel_retain(channel);

    if (channel)
    {
        err = mdv_dispatcher_post(channel->dispatcher, msg);
        mdv_channel_release(channel);
    }

    return err;
}


mdv_errno mdv_channel_recv(mdv_channel *channel)
{
    mdv_errno err = mdv_dispatcher_read(channel->dispatcher);

    switch(err)
    {
        case MDV_OK:
        case MDV_EAGAIN:
            break;

        default:
            mdv_socket_shutdown(mdv_dispatcher_fd(channel->dispatcher),
                                MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
    }

    return err;
}


mdv_errno mdv_channel_wait_connection(mdv_channel *channel, size_t timeout)
{
    if (channel->connected)
        return MDV_OK;
    return mdv_condvar_timedwait(&channel->conn_signal, timeout);
}
