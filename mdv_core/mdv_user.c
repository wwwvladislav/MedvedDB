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
        .uuid = {}
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

    mdv_errno err = mdv_tablespace_log_create_table(&user->core->storage.tablespace, 0, (mdv_table_base*)&create_table->table);

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


mdv_errno mdv_user_init(void *ctx, mdv_conctx *conctx, void *userdata)
{
    mdv_user *user = ctx;

    user->core      = userdata;
    user->conctx    = conctx;

    MDV_LOGD("User context %p initialize", user);

    mdv_dispatcher_handler const handlers[] =
    {
        { mdv_msg_hello_id,         &mdv_user_wave_handler,         user },
        { mdv_msg_create_table_id,  &mdv_user_create_table_handler, user }
    };

    for(size_t i = 0; i < sizeof handlers / sizeof *handlers; ++i)
    {
        if (mdv_dispatcher_reg(conctx->dispatcher, handlers + i) != MDV_OK)
        {
            MDV_LOGE("Messages dispatcher handler not registered");
            return MDV_FAILED;
        }
    }

    MDV_LOGD("User context %p initialized", user);

    return MDV_OK;
}


mdv_errno mdv_user_send(mdv_user *user, mdv_msg *req, mdv_msg *resp, size_t timeout)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(req->hdr.id));
    return mdv_dispatcher_send(user->conctx->dispatcher, req, resp, timeout);
}


mdv_errno mdv_user_post(mdv_user *user, mdv_msg *msg)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(msg->hdr.id));
    return mdv_dispatcher_post(user->conctx->dispatcher, msg);
}


static mdv_errno mdv_user_reply(mdv_user *user, mdv_msg const *msg)
{
    MDV_LOGI(">>>>> %s'", mdv_msg_name(msg->hdr.id));
    return mdv_dispatcher_reply(user->conctx->dispatcher, msg);
}


void mdv_user_free(void *ctx, mdv_conctx *conctx)
{
    (void)conctx;
    mdv_user *user = ctx;
    MDV_LOGD("User context %p freed", user);
}

