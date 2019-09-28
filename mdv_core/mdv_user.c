#include "mdv_user.h"
#include <mdv_messages.h>
#include <mdv_version.h>
#include <mdv_timerfd.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_socket.h>
#include <mdv_time.h>
#include <mdv_topology.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>


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


static mdv_errno mdv_user_topology_reply(mdv_user *user, uint16_t id, mdv_msg_topology const *msg)
{
    binn obj;

    if (!mdv_binn_topology(msg, &obj))
        return MDV_FAILED;

    mdv_msg message =
    {
        .hdr =
        {
            .id = mdv_msg_topology_id,
            .number = id,
            .size = binn_size(&obj)
        },
        .payload = binn_ptr(&obj)
    };

    mdv_errno err = mdv_user_reply(user, &message);

    binn_free(&obj);

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

    mdv_user    *user    = arg;
    mdv_core    *core    = user->core;
    mdv_tracker *tracker = &core->cluster.tracker;

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

    mdv_rowid const *rowid = mdv_tablespace_create_table(&user->core->storage.tablespace, (mdv_table_base*)&create_table->table);

    mdv_errno err = MDV_FAILED;

    if (rowid)
    {
        mdv_datasync_start(&core->datasync);
        mdv_committer_start(&core->committer);

        assert(rowid->peer == MDV_LOCAL_ID);

        mdv_msg_table_info const table_info =
        {
            .id =
            {
                .peer = user->core->metainf.uuid.value,
                .id = rowid->id
            }
        };

        err = mdv_user_table_info_reply(user, msg->hdr.number, &table_info);
    }
    else
    {
        mdv_msg_status const status =
        {
            .err = MDV_FAILED,
            .message = ""
        };

        err = mdv_user_status_reply(user, msg->hdr.number, &status);
    }

    mdv_free(create_table, "msg_create_table");

    return err;
}


static mdv_errno mdv_user_get_topology_handler(mdv_msg const *msg, void *arg)
{
    MDV_LOGI("<<<<< '%s'", mdv_msg_name(msg->hdr.id));

    mdv_user    *user   = arg;
    mdv_core    *core   = user->core;
    mdv_tracker *tracker = &core->cluster.tracker;

    mdv_topology *topology = mdv_topology_extract(tracker);

    if (topology)
    {
        mdv_msg_topology const msg_topology =
        {
            .topology = topology
        };

        mdv_errno err = mdv_user_topology_reply(user, msg->hdr.number, &msg_topology);

        mdv_topology_free(topology);

        return err;
    }

    mdv_msg_status const status =
    {
        .err = MDV_FAILED,
        .message = "Topology getting failed"
    };

    return mdv_user_status_reply(user, msg->hdr.number, &status);
}


mdv_errno mdv_user_init(void *ctx, mdv_conctx *conctx, void *userdata)
{
    mdv_user *user = ctx;

    user->core      = userdata;
    user->conctx    = conctx;

    MDV_LOGD("User context %p initialize", user);

    mdv_dispatcher_handler const handlers[] =
    {
        { mdv_message_id(hello),         &mdv_user_wave_handler,         user },
        { mdv_message_id(create_table),  &mdv_user_create_table_handler, user },
        { mdv_message_id(get_topology),  &mdv_user_get_topology_handler, user },
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


static mdv_errno mdv_user_reply(mdv_user *user, mdv_msg const *msg)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(msg->hdr.id));
    return mdv_dispatcher_reply(user->conctx->dispatcher, msg);
}


void mdv_user_free(void *ctx, mdv_conctx *conctx)
{
    (void)conctx;
    mdv_user *user = ctx;
    MDV_LOGD("User context %p freed", user);
}

