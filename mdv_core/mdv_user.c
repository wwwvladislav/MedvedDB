#include "mdv_user.h"
#include "mdv_config.h"
#include "event/mdv_evt_types.h"
#include "event/mdv_evt_topology.h"
#include "event/mdv_evt_table.h"
#include "event/mdv_evt_rowdata.h"
#include "event/mdv_evt_view.h"
#include "event/mdv_evt_status.h"
#include <mdv_messages.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_dispatcher.h>
#include <mdv_rollbacker.h>
#include <mdv_proto.h>
#include <mdv_mutex.h>
#include <mdv_safeptr.h>
#include <mdv_serialization.h>
#include <mdv_uuid.h>
#include <mdv_version.h>
#include <stdatomic.h>


typedef struct
{
    mdv_channel             base;           ///< connection context base type
    atomic_uint             rc;             ///< References counter
    mdv_uuid                session;        ///< Current session identifier
    mdv_uuid                uuid;           ///< Current node uuid
    mdv_dispatcher         *dispatcher;     ///< Messages dispatcher
    mdv_ebus               *ebus;           ///< Events bus
    mdv_mutex               topomutex;      ///< Mutex for topology guard
    mdv_safeptr            *topology;       ///< Current network topology
} mdv_user;


/**
 * @brief Send message and wait response.
 *
 * @param user [in]     user context
 * @param req [in]      request to be sent
 * @param resp [out]    received response
 * @param timeout [in]  timeout for response wait (in milliseconds)
 *
 * @return MDV_OK if message is successfully sent and 'resp' contains response from remote user
 * @return MDV_BUSY if there is no free slots for request. At this case caller should wait and try again later.
 * @return On error return nonzero error code
 */
static mdv_errno mdv_user_send(mdv_user *user, mdv_msg *req, mdv_msg *resp, size_t timeout);


/**
 * @brief Send message but response isn't required.
 *
 * @param user [in]     user context
 * @param msg [in]      message to be sent
 *
 * @return On success returns MDV_OK
 * @return On error return nonzero error code.
 */
static mdv_errno mdv_user_post(mdv_user *user, mdv_msg *msg);


/**
 * @brief Frees user connection context
 *
 * @param user [in]     user connection context
 */
static void mdv_user_free(mdv_user *user);


static mdv_channel * mdv_user_retain_impl(mdv_channel *channel)
{
    if (channel)
    {
        mdv_user *user = (mdv_user*)channel;

        uint32_t rc = atomic_load_explicit(&user->rc, memory_order_relaxed);

        if (!rc)
            return 0;

        while(!atomic_compare_exchange_weak(&user->rc, &rc, rc + 1))
        {
            if (!rc)
                return 0;
        }
    }

    return channel;
}


static mdv_user * mdv_user_retain(mdv_user *user)
{
    return (mdv_user *)mdv_user_retain_impl(&user->base);
}

static uint32_t mdv_user_release_impl(mdv_channel *channel)
{
    if (channel)
    {
        mdv_user *user = (mdv_user*)channel;

        uint32_t rc = atomic_fetch_sub_explicit(&user->rc, 1, memory_order_relaxed) - 1;

        if (!rc)
            mdv_user_free(user);

        return rc;
    }

    return 0;
}


static uint32_t mdv_user_release(mdv_user *user)
{
    return mdv_user_release_impl(&user->base);
}


static mdv_channel_t mdv_user_type_impl(mdv_channel const *channel)
{
    (void)channel;
    return MDV_USER_CHANNEL;
}


static mdv_uuid const * mdv_user_id_impl(mdv_channel const *channel)
{
    mdv_user *user = (mdv_user*)channel;
    return &user->session;
}


static mdv_errno mdv_user_recv_impl(mdv_channel *channel)
{
    mdv_user *user = (mdv_user*)channel;
    return mdv_dispatcher_read(user->dispatcher);
}


static mdv_errno mdv_user_reply(mdv_user *user, mdv_msg const *msg);


static mdv_errno mdv_user_status_reply(mdv_user *user, uint16_t id, mdv_msg_status const *msg)
{
    binn status;

    if (!mdv_msg_status_binn(msg, &status))
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


static mdv_errno mdv_user_view_reply(mdv_user *user, uint16_t id, mdv_msg_view const *msg)
{
    binn view;

    if (!mdv_msg_view_binn(msg, &view))
        return MDV_FAILED;

    mdv_msg message =
    {
        .hdr =
        {
            .id = mdv_msg_view_id,
            .number = id,
            .size = binn_size(&view)
        },
        .payload = binn_ptr(&view)
    };

    mdv_errno err = mdv_user_reply(user, &message);

    binn_free(&view);

    return err;
}


static mdv_errno mdv_user_view_data_reply(mdv_user *user, uint16_t id, mdv_msg_rowset const *msg)
{
    binn rowset;

    if (!mdv_msg_rowset_binn(msg, &rowset))
        return MDV_FAILED;

    mdv_msg message =
    {
        .hdr =
        {
            .id = mdv_msg_rowset_id,
            .number = id,
            .size = binn_size(&rowset)
        },
        .payload = binn_ptr(&rowset)
    };

    mdv_errno err = mdv_user_reply(user, &message);

    binn_free(&rowset);

    return err;
}


static mdv_errno mdv_user_table_info_reply(mdv_user *user, uint16_t id, mdv_msg_table_info const *msg)
{
    binn table_info;

    if (!mdv_msg_table_info_binn(msg, &table_info))
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


static mdv_errno mdv_user_table_desc_reply(mdv_user *user, uint16_t id, mdv_msg_table_desc const *msg)
{
    binn table_desc;

    if (!mdv_binn_table_desc(msg->desc, &table_desc))
        return MDV_FAILED;

    mdv_msg message =
    {
        .hdr =
        {
            .id = mdv_msg_table_desc_id,
            .number = id,
            .size = binn_size(&table_desc)
        },
        .payload = binn_ptr(&table_desc)
    };

    mdv_errno err = mdv_user_reply(user, &message);

    binn_free(&table_desc);

    return err;
}


static mdv_errno mdv_user_topology_reply(mdv_user *user, uint16_t id, mdv_msg_topology const *msg)
{
    binn obj;

    if (!mdv_msg_topology_binn(msg, &obj))
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

    mdv_msg_create_table create_table;

    mdv_errno err = MDV_FAILED;

    if (mdv_msg_create_table_unbinn(&binn_msg, &create_table))
    {
        mdv_evt_create_table *evt = mdv_evt_create_table_create(&create_table.desc);

        if (evt)
        {
            err = mdv_ebus_publish(user->ebus, &evt->base, MDV_EVT_SYNC);

            if (err == MDV_OK)
            {
                mdv_msg_table_info const table_info =
                {
                    .id = evt->table_id
                };

                err = mdv_user_table_info_reply(user, msg->hdr.number, &table_info);
            }

            mdv_evt_create_table_release(evt);
        }

        mdv_msg_create_table_free(&create_table);
    }
    else
        MDV_LOGE("Invalid '%s' message", mdv_msg_name(mdv_msg_create_table_id));

    binn_free(&binn_msg);

    if (err != MDV_OK)
    {
        mdv_msg_status const status =
        {
            .err = err,
            .message = ""
        };

        err = mdv_user_status_reply(user, msg->hdr.number, &status);
    }

    return err;
}


static mdv_errno mdv_user_get_table_handler(mdv_msg const *msg, void *arg)
{
    MDV_LOGI("<<<<< '%s'", mdv_msg_name(msg->hdr.id));

    mdv_user *user = arg;

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_msg_name(msg->hdr.id));
        return MDV_FAILED;
    }

    mdv_msg_get_table get_table;

    mdv_errno err = MDV_FAILED;

    if (mdv_msg_get_table_unbinn(&binn_msg, &get_table))
    {
        mdv_evt_table *evt = mdv_evt_table_create(&get_table.id);

        if (evt)
        {
            err = mdv_ebus_publish(user->ebus, &evt->base, MDV_EVT_SYNC);

            if (err == MDV_OK)
            {
                mdv_msg_table_desc const table_desc =
                {
                    .desc = mdv_table_description(evt->table)
                };

                err = mdv_user_table_desc_reply(user, msg->hdr.number, &table_desc);
            }

            mdv_evt_table_release(evt);
        }
    }
    else
        MDV_LOGE("Invalid '%s' message", mdv_msg_name(mdv_msg_get_table_id));

    binn_free(&binn_msg);

    if (err != MDV_OK)
    {
        mdv_msg_status const status =
        {
            .err = err,
            .message = ""
        };

        err = mdv_user_status_reply(user, msg->hdr.number, &status);
    }

    return err;
}


static mdv_errno mdv_user_insert_into_handler(mdv_msg const *msg, void *arg)
{
    MDV_LOGI("<<<<< '%s'", mdv_msg_name(msg->hdr.id));

    mdv_user    *user   = arg;

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_msg_name(msg->hdr.id));
        return MDV_FAILED;
    }

    mdv_msg_insert_into insert_into;

    mdv_errno err = MDV_FAILED;

    if (mdv_msg_insert_into_unbinn(&binn_msg, &insert_into))
    {
        mdv_evt_rowdata_ins_req *evt = mdv_evt_rowdata_ins_req_create(&insert_into.table, insert_into.rows);

        if (evt)
        {
            err = mdv_ebus_publish(user->ebus, &evt->base, MDV_EVT_SYNC);
            mdv_evt_rowdata_ins_req_release(evt);
        }
    }
    else
        MDV_LOGE("Invalid '%s' message", mdv_msg_name(mdv_msg_insert_into_id));

    binn_free(&binn_msg);

    mdv_msg_status const status =
    {
        .err = err,
        .message = ""
    };

    err = mdv_user_status_reply(user, msg->hdr.number, &status);

    return err;
}


static mdv_errno mdv_user_delete_from_handler(mdv_msg const *msg, void *arg)
{
    MDV_LOGI("<<<<< '%s'", mdv_msg_name(msg->hdr.id));

    mdv_user    *user   = arg;

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_msg_name(msg->hdr.id));
        return MDV_FAILED;
    }

    mdv_msg_delete_from delete_from;

    mdv_errno err = MDV_FAILED;

    if (mdv_msg_delete_from_unbinn(&binn_msg, &delete_from))
    {
        // TODO
    }
    else
        MDV_LOGE("Invalid '%s' message", mdv_msg_name(mdv_msg_delete_from_id));

    binn_free(&binn_msg);

    mdv_msg_status const status =
    {
        .err = err,
        .message = ""
    };

    err = mdv_user_status_reply(user, msg->hdr.number, &status);

    return err;
}


static mdv_errno mdv_user_get_topology_handler(mdv_msg const *msg, void *arg)
{
    MDV_LOGI("<<<<< '%s'", mdv_msg_name(msg->hdr.id));

    mdv_user    *user   = arg;

    mdv_topology *topology = mdv_safeptr_get(user->topology);

    if (topology)
    {
        mdv_msg_topology const msg_topology =
        {
            .topology = topology
        };

        mdv_errno err = mdv_user_topology_reply(user, msg->hdr.number, &msg_topology);

        mdv_topology_release(topology);

        return err;
    }

    mdv_msg_status const status =
    {
        .err = MDV_FAILED,
        .message = "Topology getting failed"
    };

    return mdv_user_status_reply(user, msg->hdr.number, &status);
}


static mdv_errno mdv_user_select_handler(mdv_msg const *msg, void *arg)
{
    MDV_LOGI("<<<<< '%s'", mdv_msg_name(msg->hdr.id));

    mdv_user    *user   = arg;

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_msg_name(msg->hdr.id));
        return MDV_FAILED;
    }

    mdv_msg_select select;

    mdv_errno err = MDV_FAILED;

    if (mdv_msg_select_unbinn(&binn_msg, &select))
    {
        mdv_evt_select * evt = mdv_evt_select_create(&user->session,
                                                     msg->hdr.number,
                                                     &select.table,
                                                     select.fields,
                                                     select.filter);

        if (evt)
        {
            err = mdv_ebus_publish(user->ebus, &evt->base, MDV_EVT_DEFAULT);
            mdv_evt_select_release(evt);
        }

        mdv_msg_select_free(&select);
    }
    else
        MDV_LOGE("Invalid '%s' message", mdv_msg_name(mdv_msg_select_id));

    binn_free(&binn_msg);

    if (err != MDV_OK)
    {
        mdv_msg_status const status =
        {
            .err = err,
            .message = ""
        };

        err = mdv_user_status_reply(user, msg->hdr.number, &status);
    }

    return err;
}


static mdv_errno mdv_user_fetch_handler(mdv_msg const *msg, void *arg)
{
    MDV_LOGI("<<<<< '%s'", mdv_msg_name(msg->hdr.id));

    mdv_user    *user   = arg;

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_msg_name(msg->hdr.id));
        return MDV_FAILED;
    }

    mdv_msg_fetch fetch;

    mdv_errno err = MDV_FAILED;

    if (mdv_msg_fetch_unbinn(&binn_msg, &fetch))
    {
        mdv_evt_view_fetch * evt = mdv_evt_view_fetch_create(&user->session,
                                                             msg->hdr.number,
                                                             fetch.id);

        if (evt)
        {
            err = mdv_ebus_publish(user->ebus, &evt->base, MDV_EVT_DEFAULT);
            mdv_evt_view_fetch_release(evt);
        }
    }
    else
        MDV_LOGE("Invalid '%s' message", mdv_msg_name(mdv_msg_fetch_id));

    binn_free(&binn_msg);

    if (err != MDV_OK)
    {
        mdv_msg_status const status =
        {
            .err = err,
            .message = ""
        };

        err = mdv_user_status_reply(user, msg->hdr.number, &status);
    }

    return err;
}


static mdv_errno mdv_user_evt_topology(void *arg, mdv_event *event)
{
    mdv_user *user = arg;
    mdv_evt_topology *topo = (mdv_evt_topology *)event;
    return mdv_safeptr_set(user->topology, topo->topology);
}


static mdv_errno mdv_user_evt_status(void *arg, mdv_event *event)
{
    mdv_user *user = arg;
    mdv_evt_status *evt = (mdv_evt_status *)event;

    if (mdv_uuid_cmp(&evt->session, &user->session) != 0)
        return MDV_OK;

    mdv_msg_status const status =
    {
        .err = evt->err,
        .message = evt->message
    };

    return mdv_user_status_reply(user, evt->request_id, &status);
}


static mdv_errno mdv_user_evt_view(void *arg, mdv_event *event)
{
    mdv_user *user = arg;
    mdv_evt_view *evt = (mdv_evt_view *)event;

    if (mdv_uuid_cmp(&evt->session, &user->session) != 0)
        return MDV_OK;

    mdv_msg_view const view =
    {
        .id = evt->view_id
    };

    return mdv_user_view_reply(user, evt->request_id, &view);
}


static mdv_errno mdv_user_evt_view_data(void *arg, mdv_event *event)
{
    mdv_user *user = arg;
    mdv_evt_view_data *evt = (mdv_evt_view_data *)event;

    if (mdv_uuid_cmp(&evt->session, &user->session) != 0)
        return MDV_OK;

    mdv_msg_rowset const rowset =
    {
        .rows = evt->rows
    };

    return mdv_user_view_data_reply(user, evt->request_id, &rowset);
}


static const mdv_event_handler_type mdv_user_handlers[] =
{
    { MDV_EVT_TOPOLOGY,     mdv_user_evt_topology },
    { MDV_EVT_STATUS,       mdv_user_evt_status },
    { MDV_EVT_VIEW,         mdv_user_evt_view },
    { MDV_EVT_VIEW_DATA,    mdv_user_evt_view_data },
};


mdv_channel * mdv_user_create(mdv_descriptor  fd,
                              mdv_uuid const *uuid,
                              mdv_uuid const *session,
                              mdv_ebus       *ebus,
                              mdv_topology   *topology)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    mdv_user *user = mdv_alloc(sizeof(mdv_user), "userctx");

    if (!user)
    {
        MDV_LOGE("No memory for user connection context");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, user, "userctx");

    MDV_LOGD("User %p initialization", user);

    static const mdv_ichannel vtbl =
    {
        .retain = mdv_user_retain_impl,
        .release = mdv_user_release_impl,
        .type = mdv_user_type_impl,
        .id = mdv_user_id_impl,
        .recv = mdv_user_recv_impl,
    };

    user->base.vptr = &vtbl;

    atomic_init(&user->rc, 1);

    user->session = *session;
    user->uuid = *uuid;

    user->ebus = mdv_ebus_retain(ebus);
    mdv_rollbacker_push(rollbacker, mdv_ebus_release, user->ebus);

    user->topology = mdv_safeptr_create(topology,
                                        (mdv_safeptr_retain_fn)mdv_topology_retain,
                                        (mdv_safeptr_release_fn)mdv_topology_release);

    if (!user->topology)
    {
        MDV_LOGE("Safe pointer creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_safeptr_free, user->topology);

    user->dispatcher = mdv_dispatcher_create(fd);

    if (!user->dispatcher)
    {
        MDV_LOGE("User connection context creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_dispatcher_free, user->dispatcher);

    mdv_dispatcher_handler const handlers[] =
    {
        { mdv_message_id(create_table),  &mdv_user_create_table_handler, user },
        { mdv_message_id(get_table),     &mdv_user_get_table_handler,    user },
        { mdv_message_id(insert_into),   &mdv_user_insert_into_handler,  user },
        { mdv_message_id(get_topology),  &mdv_user_get_topology_handler, user },
        { mdv_message_id(select),        &mdv_user_select_handler,       user },
        { mdv_message_id(fetch),         &mdv_user_fetch_handler,        user },
        { mdv_message_id(delete_from),   &mdv_user_delete_from_handler,  user },

    };

    for(size_t i = 0; i < sizeof handlers / sizeof *handlers; ++i)
    {
        if (mdv_dispatcher_reg(user->dispatcher, handlers + i) != MDV_OK)
        {
            MDV_LOGE("Messages dispatcher handler not registered");
            mdv_rollback(rollbacker);
            return 0;
        }
    }

    if (mdv_ebus_subscribe_all(user->ebus,
                               user,
                               mdv_user_handlers,
                               sizeof mdv_user_handlers / sizeof *mdv_user_handlers) != MDV_OK)
    {
        MDV_LOGE("Ebus subscription failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    MDV_LOGD("User %p initialized", user);

    mdv_rollbacker_free(rollbacker);

    return &user->base;
}


static void mdv_user_free(mdv_user *user)
{
    if(user)
    {
        mdv_ebus_unsubscribe_all(user->ebus,
                                 user,
                                 mdv_user_handlers,
                                 sizeof mdv_user_handlers / sizeof *mdv_user_handlers);
        mdv_dispatcher_free(user->dispatcher);
        mdv_safeptr_free(user->topology);
        mdv_ebus_release(user->ebus);
        mdv_free(user, "userctx");
        MDV_LOGD("User %p freed", user);
    }
}


static mdv_errno mdv_user_send(mdv_user *user, mdv_msg *req, mdv_msg *resp, size_t timeout)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(req->hdr.id));

    mdv_errno err = MDV_CLOSED;

    user = mdv_user_retain(user);

    if (user)
    {
        err = mdv_dispatcher_send(user->dispatcher, req, resp, timeout);
        mdv_user_release(user);
    }

    return err;
}


static mdv_errno mdv_user_post(mdv_user *user, mdv_msg *msg)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(msg->hdr.id));

    mdv_errno err = MDV_CLOSED;

    user = mdv_user_retain(user);

    if (user)
    {
        err = mdv_dispatcher_post(user->dispatcher, msg);
        mdv_user_release(user);
    }

    return err;
}


static mdv_errno mdv_user_reply(mdv_user *user, mdv_msg const *msg)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(msg->hdr.id));
    return mdv_dispatcher_reply(user->dispatcher, msg);
}

