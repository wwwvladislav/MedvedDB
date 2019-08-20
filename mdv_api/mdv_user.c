#include "mdv_user.h"
#include "mdv_messages.h"
#include <mdv_version.h>
#include <mdv_log.h>


/**
 * @brief   Say Hey! to server
 */
static mdv_errno mdv_user_wave(mdv_user *user)
{
    mdv_msg_hello const hello =
    {
        .uuid    = {},
        .version = MDV_VERSION
    };

    binn hey;

    if (!mdv_binn_hello(&hello, &hey))
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

    mdv_errno err = mdv_user_post(user, &message);

    binn_free(&hey);

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

    user->uuid = client_hello.uuid;

    user->userdata->user = user;

    return mdv_condvar_signal(user->userdata->connected);
}


static mdv_errno mdv_user_status_handler(mdv_msg const *msg, void *arg)
{
    MDV_LOGI("<<<<< '%s'", mdv_msg_name(msg->hdr.id));
    (void)msg;
    (void)arg;
    return MDV_NO_IMPL;
}


mdv_errno mdv_user_init(void *ctx, mdv_conctx *conctx, void *userdata)
{
    mdv_user *user  = ctx;

    user->conctx    = conctx;
    user->userdata  = userdata;

    MDV_LOGD("User context %p initialize", user);

    mdv_dispatcher_handler const handlers[] =
    {
        { mdv_message_id(hello),    &mdv_user_wave_handler,   user },
        { mdv_message_id(status),   &mdv_user_status_handler, user }
    };

    for(size_t i = 0; i < sizeof handlers / sizeof *handlers; ++i)
    {
        if (mdv_dispatcher_reg(conctx->dispatcher, handlers + i) != MDV_OK)
        {
            MDV_LOGE("Messages dispatcher handler not registered");
            return MDV_FAILED;
        }
    }

    if (mdv_user_wave(user) != MDV_OK)
    {
        MDV_LOGD("User handshake message failed");
        return MDV_FAILED;
    }

    MDV_LOGD("User context %p initialized", user);

    return MDV_OK;
}


mdv_errno mdv_user_send(mdv_user *user, mdv_msg *req, mdv_msg *resp, size_t timeout)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(req->hdr.id));

    mdv_errno err = MDV_CLOSED;

    mdv_conctx *conctx = mdv_cluster_conctx_retain(user->conctx);

    if (conctx)
    {
        err = mdv_dispatcher_send(user->conctx->dispatcher, req, resp, timeout);
        mdv_cluster_conctx_release(conctx);
    }

    return err;
}


mdv_errno mdv_user_post(mdv_user *user, mdv_msg *msg)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(msg->hdr.id));

    mdv_errno err = MDV_CLOSED;

    mdv_conctx *conctx = mdv_cluster_conctx_retain(user->conctx);

    if (conctx)
    {
        err = mdv_dispatcher_post(user->conctx->dispatcher, msg);
        mdv_cluster_conctx_release(conctx);
    }

    return err;
}


void mdv_user_free(void *ctx, mdv_conctx *conctx)
{
    mdv_user *user = ctx;
    MDV_LOGD("User context %p freed", user);
}

