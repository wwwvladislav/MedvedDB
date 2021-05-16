#include "mdv_connection.h"
#include "mdv_messages.h"
#include "mdv_proto.h"
#include <mdv_log.h>
#include <mdv_uuid.h>
#include <mdv_dispatcher.h>
#include <mdv_socket.h>
#include <mdv_rollbacker.h>
#include <mdv_version.h>
#include <stdatomic.h>


/// User connection context used for storing different type of information
/// about connection
struct mdv_connection
{
    mdv_channel             base;           ///< connection context base type
    atomic_uint_fast32_t    rc;             ///< references counter
    mdv_uuid                uuid;           ///< server uuid
    mdv_dispatcher         *dispatcher;     ///< Messages dispatcher
};


static mdv_channel * mdv_connection_retain_impl(mdv_channel *channel)
{
    mdv_connection *con = (mdv_connection*)channel;
    atomic_fetch_add_explicit(&con->rc, 1, memory_order_acquire);
    return channel;
}


static void mdv_connection_free(mdv_connection *con)
{
    mdv_dispatcher_free(con->dispatcher);
    mdv_free(con);
    MDV_LOGD("Connection %p freed", con);
}


static uint32_t mdv_connection_release_impl(mdv_channel *channel)
{
    mdv_connection *con = (mdv_connection*)channel;

    uint32_t rc = 0;

    if (channel)
    {
        rc = atomic_fetch_sub_explicit(&con->rc, 1, memory_order_release) - 1;
        if (!rc)
            mdv_connection_free(con);
    }

    return rc;
}


static mdv_channel_t mdv_connection_type_impl(mdv_channel const *channel)
{
    (void)channel;
    return MDV_USER_CHANNEL;
}


static mdv_uuid const * mdv_connection_id_impl(mdv_channel const *channel)
{
    mdv_connection *con = (mdv_connection*)channel;
    return &con->uuid;
}


static mdv_errno mdv_connection_recv_impl(mdv_channel *channel)
{
    mdv_connection *con = (mdv_connection*)channel;
    return mdv_dispatcher_read(con->dispatcher);
}


static mdv_errno mdv_channel_status_handler(mdv_msg const *msg, void *arg)
{
    MDV_LOGI("<<<<< '%s'", mdv_msg_name(msg->hdr.id));
    (void)msg;
    (void)arg;
    return MDV_NO_IMPL;
}


mdv_connection * mdv_connection_create(mdv_descriptor fd, mdv_uuid const *uuid)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    mdv_connection *con = mdv_alloc(sizeof(mdv_connection));

    if (!con)
    {
        MDV_LOGE("No memory for user connection context");
        mdv_rollback(rollbacker);
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, con);

    MDV_LOGD("Connection %p initialization", con);

    static const mdv_ichannel vtbl =
    {
        .retain = mdv_connection_retain_impl,
        .release = mdv_connection_release_impl,
        .type = mdv_connection_type_impl,
        .id = mdv_connection_id_impl,
        .recv = mdv_connection_recv_impl,
    };

    con->base.vptr = &vtbl;

    atomic_init(&con->rc, 1);

    con->uuid = *uuid;

    con->dispatcher = mdv_dispatcher_create(fd);

    if (!con->dispatcher)
    {
        MDV_LOGE("Messages dispatcher not created");
        mdv_rollback(rollbacker);
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_dispatcher_free, con->dispatcher);

    mdv_dispatcher_handler const handlers[] =
    {
        { mdv_message_id(status),   &mdv_channel_status_handler, con }
    };

    for(size_t i = 0; i < sizeof handlers / sizeof *handlers; ++i)
    {
        if (mdv_dispatcher_reg(con->dispatcher, handlers + i) != MDV_OK)
        {
            MDV_LOGE("Messages dispatcher handler not registered");
            mdv_rollback(rollbacker);
            mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
            return 0;
        }
    }

    MDV_LOGD("Connection context %p initialized", con);

    mdv_rollbacker_free(rollbacker);

    return con;
}


mdv_connection * mdv_connection_retain(mdv_connection *con)
{
    return (mdv_connection *)mdv_connection_retain_impl(&con->base);
}


uint32_t mdv_connection_release(mdv_connection *con)
{
    return mdv_connection_release_impl(&con->base);
}


mdv_uuid const * mdv_connection_uuid(mdv_connection *con)
{
    return mdv_connection_id_impl(&con->base);
}


mdv_errno mdv_connection_send(mdv_connection *con, mdv_msg *req, mdv_msg *resp, size_t timeout)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(req->hdr.id));

    mdv_errno err = MDV_CLOSED;

    con = mdv_connection_retain(con);

    if (con)
    {
        err = mdv_dispatcher_send(con->dispatcher, req, resp, timeout);
        mdv_connection_release(con);
    }

    return err;
}


mdv_errno mdv_connection_post(mdv_connection *con, mdv_msg *msg)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(msg->hdr.id));

    mdv_errno err = MDV_CLOSED;

    con = mdv_connection_retain(con);

    if (con)
    {
        err = mdv_dispatcher_post(con->dispatcher, msg);
        mdv_connection_release(con);
    }

    return err;
}
