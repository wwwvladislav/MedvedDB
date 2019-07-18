#include "mdv_user.h"
#include <mdv_messages.h>
#include <mdv_version.h>
#include <mdv_timerfd.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_socket.h>
#include <mdv_time.h>
#include <stddef.h>
#include <string.h>


/// User context used for storing different type of information about connection (it should be cast to mdv_conctx)
typedef struct mdv_user
{
    mdv_cli_type        type;                   ///< Client type
    mdv_tablespace     *tablespace;             ///< tablespace
    mdv_dispatcher     *dispatcher;             ///< Messages dispatcher
    mdv_descriptor      sock;                   ///< Socket associated with connection
    mdv_uuid            current_uuid;           ///< current node uuid
    size_t              created_time;           ///< time, when user connected
} mdv_user;


static mdv_errno mdv_user_reply(mdv_user *user, mdv_msg const *msg);


static mdv_errno mdv_user_wave_reply(mdv_user *user, uint16_t id, mdv_msg_hello const *msg)
{
    binn hey;

    if (!mdv_binn_hello(msg, &hey))
        return MDV_FAILED;

    mdv_msg message =
    {
        .hdr =
        {
            .id     = mdv_msg_hello_id,
            .number = id,
            .size   = binn_size(&hey)
        },
        .payload = binn_ptr(&hey)
    };

    mdv_errno err = mdv_user_reply(user, &message);

    binn_free(&hey);

    return err;
}


static mdv_errno mdv_user_status_reply(mdv_user *user, uint16_t id, mdv_msg_status const *msg)
{
    binn status;

    if (!mdv_binn_status(msg, &status))
        return MDV_FAILED;

    mdv_msg message =
    {
        .hdr =
        {
            .id = mdv_msg_status_id,
            .number = id,
            .size = binn_size(&status)
        },
        .payload = binn_ptr(&status)
    };

    mdv_errno err = mdv_user_reply(user, &message);

    binn_free(&status);

    return err;
}


static mdv_errno mdv_user_table_info_reply(mdv_user *user, uint16_t id, mdv_msg_table_info const *msg)
{
    binn table_info;

    if (!mdv_binn_table_info(msg, &table_info))
        return MDV_FAILED;

    mdv_msg message =
    {
        .hdr =
        {
            .id = mdv_msg_table_info_id,
            .number = id,
            .size = binn_size(&table_info)
        },
        .payload = binn_ptr(&table_info)
    };

    mdv_errno err = mdv_user_reply(user, &message);

    binn_free(&table_info);

    return err;
}


static mdv_errno mdv_user_wave_handler(mdv_msg const *msg, void *arg)
{
    MDV_LOGI("<<<<< '%s'", mdv_msg_name(msg->hdr.id));

    mdv_user *user = arg;

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_msg_name(msg->hdr.id));
        return MDV_FAILED;
    }

    mdv_msg_hello client_hello = {};

    if (!mdv_unbinn_hello(&binn_msg, &client_hello))
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

    mdv_msg_hello hello =
    {
        .version = MDV_VERSION,
        .uuid = user->current_uuid
    };

    return mdv_user_wave_reply(user, msg->hdr.number, &hello);
}


static mdv_errno mdv_user_create_table_handler(mdv_msg const *msg, void *arg)
{
    MDV_LOGI("<<<<< '%s'", mdv_msg_name(msg->hdr.id));

    mdv_user *user = arg;

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_msg_name(msg->hdr.id));
        return MDV_FAILED;
    }

    mdv_msg_create_table_base *create_table = mdv_unbinn_create_table(&binn_msg);

    binn_free(&binn_msg);

    if (!create_table)
    {
        MDV_LOGE("Invalid '%s' message", mdv_msg_name(mdv_msg_create_table_id));
        return MDV_FAILED;
    }

    mdv_errno err = mdv_tablespace_log_create_table(user->tablespace, 0, (mdv_table_base*)&create_table->table);

    if (err == MDV_OK)
    {
        mdv_msg_table_info const table_info =
        {
            .uuid = create_table->table.uuid
        };

        err = mdv_user_table_info_reply(user, msg->hdr.number, &table_info);
    }
    else
    {
        mdv_msg_status const status =
        {
            .err = err,
            .message = { 0 }
        };

        err = mdv_user_status_reply(user, msg->hdr.number, &status);
    }

    mdv_free(create_table, "msg_create_table");

    return err;
}


mdv_user * mdv_user_accept(mdv_tablespace *tablespace, mdv_descriptor fd, mdv_uuid const *uuid)
{
    mdv_user *user = mdv_alloc(sizeof(mdv_user), "user");

    if (!user)
    {
        MDV_LOGE("No memory to accept user context");
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    MDV_LOGD("User context %p initialize", user);

    user->type          = MDV_CLI_USER;
    user->tablespace    = tablespace;
    user->sock          = fd;
    user->current_uuid  = *uuid;
    user->created_time  = mdv_gettime();

    mdv_dispatcher_handler const handlers[] =
    {
        { mdv_msg_hello_id,         &mdv_user_wave_handler,         user },
        { mdv_msg_create_table_id,  &mdv_user_create_table_handler, user }
    };

    user->dispatcher = mdv_dispatcher_create(fd, sizeof handlers / sizeof *handlers, handlers);

    if (!user->dispatcher)
    {
        MDV_LOGE("Messages dispatcher not created");
        mdv_free(user, "user");
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    MDV_LOGD("User context %p accepted", user);

    return user;
}


mdv_errno mdv_user_recv(mdv_user *user)
{
    mdv_errno err = mdv_dispatcher_read(user->dispatcher);

    switch(err)
    {
        case MDV_OK:
        case MDV_EAGAIN:
            break;

        default:
            mdv_socket_shutdown(user->sock, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
    }

    return err;
}


mdv_errno mdv_user_send(mdv_user *user, mdv_msg *req, mdv_msg *resp, size_t timeout)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(req->hdr.id));
    return mdv_dispatcher_send(user->dispatcher, req, resp, timeout);
}


mdv_errno mdv_user_post(mdv_user *user, mdv_msg *msg)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(msg->hdr.id));
    return mdv_dispatcher_post(user->dispatcher, msg);
}


static mdv_errno mdv_user_reply(mdv_user *user, mdv_msg const *msg)
{
    MDV_LOGI(">>>>> %s'", mdv_msg_name(msg->hdr.id));
    return mdv_dispatcher_reply(user->dispatcher, msg);
}


void mdv_user_free(mdv_user *user)
{
    MDV_LOGD("User context %p freed", user);
    mdv_dispatcher_free(user->dispatcher);
    mdv_free(user, "user");
}

