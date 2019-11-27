#include "mdv_datasync.h"
#include "event/mdv_evt_types.h"
#include "event/mdv_evt_topology.h"
#include "event/mdv_evt_trlog.h"
#include <stdatomic.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <mdv_eventfd.h>
#include <mdv_threads.h>
#include <mdv_safeptr.h>
#include <mdv_router.h>


/// Data synchronizer
struct mdv_datasync
{
    atomic_uint     rc;             ///< References counter
    mdv_ebus       *ebus;           ///< Event bus
    mdv_jobber     *jobber;         ///< Jobs scheduler
    mdv_descriptor  start;          ///< Signal for starting
    mdv_thread      thread;         ///< Committer thread
    atomic_bool     active;         ///< Status
    atomic_size_t   active_jobs;    ///< Active jobs counter
    mdv_safeptr    *topology;       ///< Current network topology
    mdv_safeptr    *routes;         ///< Routes for data synchronization
    mdv_uuid        uuid;           ///< Current node UUID
};


typedef struct mdv_datasync_context
{
    mdv_datasync   *datasync;       ///< Data synchronizer
    mdv_hashmap    *routes;         ///< Destination peers
    mdv_uuid        trlog;          ///< TR log UUID for synchronization
} mdv_datasync_context;


typedef mdv_job(mdv_datasync_context)     mdv_datasync_job;


static bool mdv_datasync_is_active(mdv_datasync *datasync)
{
    return atomic_load(&datasync->active);
}


static mdv_trlog * mdv_datasync_trlog(mdv_datasync *datasync, mdv_uuid const *uuid)
{
    mdv_evt_trlog *evt = mdv_evt_trlog_create(uuid);

    if (!evt)
    {
        MDV_LOGE("Transaction log request failed. No memory.");
        return 0;
    }

    if (mdv_ebus_publish(datasync->ebus, &evt->base, MDV_EVT_SYNC) != MDV_OK)
        MDV_LOGE("Transaction log request failed");

    mdv_trlog *trlog = mdv_trlog_retain(evt->trlog);

    mdv_evt_trlog_release(evt);

    return trlog;
}


static void mdv_datasync_fn(mdv_job_base *job)
{
    mdv_datasync_context *ctx      = (mdv_datasync_context *)job->data;
    mdv_datasync         *datasync = ctx->datasync;

    if (!mdv_datasync_is_active(datasync))
        return;

    mdv_hashmap_foreach(ctx->routes, mdv_route, route)
    {
        mdv_evt_trlog_sync *sync = mdv_evt_trlog_sync_create(&ctx->trlog, &datasync->uuid, &route->uuid);

        if (sync)
        {
            if (mdv_ebus_publish(datasync->ebus, &sync->base, MDV_EVT_SYNC) != MDV_OK)
                MDV_LOGE("Transaction synchronization failed");
            mdv_evt_trlog_sync_release(sync);
        }
        else
            MDV_LOGE("Transaction synchronization failed. No memory.");
    }
}


static void mdv_datasync_finalize(mdv_job_base *job)
{
    mdv_datasync_context *ctx = (mdv_datasync_context *)job->data;
    mdv_datasync         *datasync = ctx->datasync;
    mdv_hashmap_release(ctx->routes);
    mdv_datasync_release(datasync);
    atomic_fetch_sub_explicit(&datasync->active_jobs, 1, memory_order_relaxed);
    mdv_free(job, "datasync_job");
}


static void mdv_datasync_job_emit(mdv_datasync *datasync, mdv_hashmap *routes, mdv_uuid const *trlog)
{
    mdv_datasync_job *job = mdv_alloc(sizeof(mdv_datasync_job), "datasync_job");

    if (!job)
    {
        MDV_LOGE("No memory for data synchronization job");
        return;
    }

    job->fn             = mdv_datasync_fn;
    job->finalize       = mdv_datasync_finalize;
    job->data.datasync  = mdv_datasync_retain(datasync);
    job->data.routes    = mdv_hashmap_retain(routes);
    job->data.trlog     = *trlog;

    mdv_errno err = mdv_jobber_push(datasync->jobber, (mdv_job_base*)job);

    if (err != MDV_OK)
    {
        MDV_LOGE("Data synchronization job failed");
        mdv_hashmap_release(routes);
        mdv_datasync_release(datasync);
        mdv_free(job, "datasync_job");
    }
    else
    {
        atomic_fetch_add_explicit(&datasync->active_jobs, 1, memory_order_relaxed);
    }
}


// Main data synchronization thread generates asynchronous jobs for each peer.
static void mdv_datasync_main(mdv_datasync *datasync)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    do
    {
        mdv_topology *topology = mdv_safeptr_get(datasync->topology);

        if (!topology)
            break;

        mdv_rollbacker_push(rollbacker, mdv_topology_release, topology);

        mdv_vector *nodes = mdv_topology_nodes(topology);

        if (!nodes)
            break;

        mdv_rollbacker_push(rollbacker, mdv_vector_release, nodes);

        if (mdv_vector_empty(nodes))
            break;

        mdv_hashmap *routes = mdv_safeptr_get(datasync->routes);

        if (!routes)
            break;

        mdv_rollbacker_push(rollbacker, mdv_hashmap_release, routes);

        if (mdv_hashmap_empty(routes))
            break;

        mdv_vector_foreach(nodes, mdv_toponode, node)
        {
            mdv_datasync_job_emit(datasync, routes, &node->uuid);
        }
    }
    while(0);

    mdv_rollback(rollbacker);
}


static void * mdv_datasync_thread(void *arg)
{
    mdv_datasync *datasync = arg;

    atomic_store(&datasync->active, true);

    mdv_descriptor epfd = mdv_epoll_create();

    if (epfd == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("epoll creation failed");
        return 0;
    }

    mdv_epoll_event const event =
    {
        .events = MDV_EPOLLIN | MDV_EPOLLERR,
        .data = 0
    };

    if (mdv_epoll_add(epfd, datasync->start, event) != MDV_OK)
    {
        MDV_LOGE("epoll event registration failed");
        mdv_epoll_close(epfd);
        return 0;
    }

    while(mdv_datasync_is_active(datasync))
    {
        mdv_epoll_event events[1];
        uint32_t size = sizeof events / sizeof *events;

        mdv_epoll_wait(epfd, events, &size, -1);

        if (!mdv_datasync_is_active(datasync))
            break;

        if (size)
        {
            if (events[0].events & MDV_EPOLLIN)
            {
                uint64_t signal;
                size_t len = sizeof(signal);

                while(mdv_read(datasync->start, &signal, &len) == MDV_EAGAIN);

                // Synchronization start
                mdv_datasync_main(datasync);
            }
        }
    }

    mdv_epoll_close(epfd);

    return 0;
}


static mdv_errno mdv_datasync_evt_topology(void *arg, mdv_event *event)
{
    mdv_datasync *datasync = arg;
    mdv_evt_topology *topo = (mdv_evt_topology *)event;

    mdv_errno err = mdv_safeptr_set(datasync->topology, topo->topology);

    mdv_hashmap *routes = mdv_routes_find(topo->topology, &datasync->uuid);

    if (routes)
    {
        err = err == MDV_OK
                ? mdv_safeptr_set(datasync->routes, routes)
                : err;

        mdv_hashmap_release(routes);
    }
    else
        MDV_LOGE("Routes calculation failed");

    return err;
}


static mdv_errno mdv_datasync_evt_trlog_changed(void *arg, mdv_event *event)
{
    mdv_datasync *datasync = arg;
    (void)event;
    mdv_datasync_start(datasync);
    return MDV_OK;
}


static mdv_errno mdv_datasync_evt_trlog_sync(void *arg, mdv_event *event)
{
    mdv_datasync *datasync = arg;
    mdv_evt_trlog_sync *sync = (mdv_evt_trlog_sync *)event;

    if(mdv_uuid_cmp(&datasync->uuid, &sync->to) != 0)
        return MDV_OK;

    mdv_errno err = MDV_FAILED;

    mdv_trlog *trlog = mdv_datasync_trlog(datasync, &sync->trlog);

    mdv_evt_trlog_state *evt = mdv_evt_trlog_state_create(&sync->trlog, &sync->to, &sync->from, trlog ? mdv_trlog_top(trlog) : 0);

    if (evt)
    {
        err = mdv_ebus_publish(datasync->ebus, &evt->base, MDV_EVT_DEFAULT);
        mdv_evt_trlog_state_release(evt);
    }

    mdv_trlog_release(trlog);

    return err;
}


static mdv_errno mdv_datasync_evt_trlog_state(void *arg, mdv_event *event)
{
    mdv_datasync *datasync = arg;
    mdv_evt_trlog_state *state = (mdv_evt_trlog_state *)event;

    if(mdv_uuid_cmp(&datasync->uuid, &state->to) != 0)
        return MDV_OK;

    mdv_errno err = MDV_FAILED;

    mdv_trlog *trlog = mdv_datasync_trlog(datasync, &state->trlog);

    if (trlog)
    {
        if (mdv_trlog_top(trlog) > state->top)
        {
            // TODO: Transaction logs synchronization
            MDV_LOGE("TODO TODO TODO");
        }

        mdv_trlog_release(trlog);
    }

    return err;
}


static const mdv_event_handler_type mdv_datasync_handlers[] =
{
    { MDV_EVT_TOPOLOGY,         mdv_datasync_evt_topology },
    { MDV_EVT_TRLOG_CHANGED,    mdv_datasync_evt_trlog_changed },
    { MDV_EVT_TRLOG_SYNC,       mdv_datasync_evt_trlog_sync },
    { MDV_EVT_TRLOG_STATE,      mdv_datasync_evt_trlog_state },
};


mdv_datasync * mdv_datasync_create(mdv_uuid const *uuid,
                                   mdv_ebus *ebus,
                                   mdv_jobber_config const *jconfig,
                                   mdv_topology *topology)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(7);

    mdv_datasync *datasync = mdv_alloc(sizeof(mdv_datasync), "datasync");

    if (!datasync)
    {
        MDV_LOGE("No memory for new datasync");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, datasync, "datasync");

    atomic_init(&datasync->rc, 1);
    atomic_init(&datasync->active_jobs, 0);
    atomic_init(&datasync->active, false);

    datasync->uuid = *uuid;

    datasync->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, datasync->ebus);

    datasync->topology = mdv_safeptr_create(topology,
                                        (mdv_safeptr_retain_fn)mdv_topology_retain,
                                        (mdv_safeptr_release_fn)mdv_topology_release);

    if (!datasync->topology)
    {
        MDV_LOGE("Safe pointer creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_safeptr_free, datasync->topology);

    mdv_hashmap *routes = mdv_routes_find(topology, uuid);

    if (!routes)
    {
        MDV_LOGE("Routes calculation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    datasync->routes = mdv_safeptr_create(routes,
                                        (mdv_safeptr_retain_fn)mdv_hashmap_retain,
                                        (mdv_safeptr_release_fn)mdv_hashmap_release);

    if (!datasync->routes)
    {
        MDV_LOGE("Safe pointer creation failed");
        mdv_hashmap_release(routes);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_safeptr_free, datasync->routes);

    mdv_hashmap_release(routes);

    datasync->jobber = mdv_jobber_create(jconfig);

    if (!datasync->jobber)
    {
        MDV_LOGE("Jobs scheduler creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_jobber_release, datasync->jobber);

    datasync->start = mdv_eventfd(false);

    if (datasync->start == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("Eventfd creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_eventfd_close, datasync->start);

    mdv_thread_attrs const attrs =
    {
        .stack_size = MDV_THREAD_STACK_SIZE
    };

    mdv_errno err = mdv_thread_create(&datasync->thread, &attrs, mdv_datasync_thread, datasync);

    if (err != MDV_OK)
    {
        MDV_LOGE("Thread creation failed with error %d", err);
        mdv_rollback(rollbacker);
        return 0;
    }

    while(!mdv_datasync_is_active(datasync))
        mdv_sleep(100);

    mdv_rollbacker_push(rollbacker, mdv_datasync_stop, datasync);

    if (mdv_ebus_subscribe_all(datasync->ebus,
                               datasync,
                               mdv_datasync_handlers,
                               sizeof mdv_datasync_handlers / sizeof *mdv_datasync_handlers) != MDV_OK)
    {
        MDV_LOGE("Ebus subscription failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_free(rollbacker);

    return datasync;
}


void mdv_datasync_start(mdv_datasync *datasync)
{
    uint64_t const signal = 1;
    mdv_write_all(datasync->start, &signal, sizeof signal);
}


void mdv_datasync_stop(mdv_datasync *datasync)
{
    if (mdv_datasync_is_active(datasync))
    {
        atomic_store(&datasync->active, false);

        uint64_t const signal = 1;
        mdv_write_all(datasync->start, &signal, sizeof signal);

        mdv_thread_join(datasync->thread);

        while(atomic_load_explicit(&datasync->active_jobs, memory_order_relaxed) > 0)
            mdv_sleep(100);
    }
}


static void mdv_datasync_free(mdv_datasync *datasync)
{
    mdv_datasync_stop(datasync);

    mdv_ebus_unsubscribe_all(datasync->ebus,
                             datasync,
                             mdv_datasync_handlers,
                             sizeof mdv_datasync_handlers / sizeof *mdv_datasync_handlers);

    mdv_jobber_release(datasync->jobber);
    mdv_eventfd_close(datasync->start);
    mdv_ebus_release(datasync->ebus);
    mdv_safeptr_free(datasync->topology);
    mdv_safeptr_free(datasync->routes);
    memset(datasync, 0, sizeof(*datasync));
    mdv_free(datasync, "datasync");
}


mdv_datasync * mdv_datasync_retain(mdv_datasync *datasync)
{
    atomic_fetch_add_explicit(&datasync->rc, 1, memory_order_acquire);
    return datasync;
}


uint32_t mdv_datasync_release(mdv_datasync *datasync)
{
    if (!datasync)
        return 0;

    uint32_t rc = atomic_fetch_sub_explicit(&datasync->rc, 1, memory_order_release) - 1;

    if (!rc)
        mdv_datasync_free(datasync);

    return rc;
}


#if 0
typedef struct
{
    mdv_datasync   *datasync;       ///< Data synchronizer
    mdv_uuid        uuid;           ///< CF Storage unique identifier
    mdv_uuid        peer;           ///< Peer unique identifier
} mdv_datasync_cfs_context;


static bool mdv_datasync_cfs(void           *arg,
                             uint32_t        peer_src,
                             uint32_t        peer_dst,
                             size_t          count,
                             mdv_list const *ops)       // mdv_cfstorage_op ops[]
{
    mdv_datasync_cfs_context *ctx = arg;

    // TODO: mdv_static_assert(sizeof(mdv_cfslog_data) == sizeof(mdv_cfstorage_op));

    mdv_msg_p2p_cfslog_data cfslog_data =
    {
        .uuid = ctx->uuid,
        .peer = ctx->peer,
        .count = count,
        .rows = (mdv_list*)ops
    };

    binn obj;

    if (mdv_binn_p2p_cfslog_data(&cfslog_data, &obj))
    {
        mdv_msg message =
        {
            .hdr =
            {
                .id = mdv_message_id(p2p_cfslog_data),
                .size = binn_size(&obj)
            },
            .payload = binn_ptr(&obj)
        };

        // TODO: mdv_tracker_peers_call(ctx->datasync->tracker, peer_dst, &message, &mdv_peer_node_post);

        binn_free(&obj);
    }
    else
        return false;

    return true;
}


mdv_errno mdv_datasync_cfslog_state_handler(mdv_datasync *datasync,
                                            uint32_t peer_id,
                                            mdv_msg const *msg)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(1);

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_p2p_msg_name(msg->hdr.id));
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &binn_msg);

    mdv_msg_p2p_cfslog_state cfslog_state;

    if (!mdv_unbinn_p2p_cfslog_state(&binn_msg, &cfslog_state))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_p2p_msg_name(msg->hdr.id));
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

/* TODO
    mdv_cfstorage *storage = mdv_tablespace_cfstorage(datasync->tablespace, &cfslog_state.uuid);

    mdv_node *node = mdv_tracker_node_by_uuid(datasync->tracker, &cfslog_state.peer);

    if (storage && node)
    {
        if (cfslog_state.trlog_top < mdv_cfstorage_log_last_id(storage, node->id))
        {
            mdv_datasync_cfs_context ctx =
            {
                .datasync = datasync,
                .uuid = cfslog_state.uuid,
                .peer = cfslog_state.peer
            };

            mdv_cfstorage_sync(storage,
                            cfslog_state.trlog_top,
                            node->id,           // peer_src
                            peer_id,            // peer_dst
                            &ctx,
                            mdv_datasync_cfs);
        }
    }
    else
        MDV_LOGE("Storage or node not found. (storage: %p, node: %p)", storage, node);
*/
    mdv_rollback(rollbacker);

    return MDV_OK;
}


mdv_errno mdv_datasync_cfslog_data_handler(mdv_datasync  *datasync,
                                           uint32_t       peer_id,
                                           mdv_msg const *msg)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_p2p_msg_name(msg->hdr.id));
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &binn_msg);

    mdv_list ops = {};

    mdv_uuid const *storage_uuid = mdv_unbinn_p2p_cfslog_data_uuid(&binn_msg);
    mdv_uuid const *peer_uuid = mdv_unbinn_p2p_cfslog_data_peer(&binn_msg);
    uint32_t const *ops_count = mdv_unbinn_p2p_cfslog_data_count(&binn_msg);

    if (mdv_unbinn_p2p_cfslog_data_rows(&binn_msg, &ops))
    {
        mdv_rollbacker_push(rollbacker, mdv_list_clear, &ops);

        if (storage_uuid
            && peer_uuid
            && ops_count)
        {
/* TODO
            mdv_node const *node = mdv_tracker_node_by_uuid(datasync->tracker, peer_uuid);

            if (node)
            {

                mdv_cfstorage *storage = mdv_tablespace_cfstorage(datasync->tablespace, storage_uuid);

                if (storage)
                {
                    if (mdv_cfstorage_log_add(storage, node->id, &ops))
                        mdv_datasync_start(datasync);
                    else
                        MDV_LOGE("cfstorage_log_add failed");
                }
                else
                    MDV_LOGE("Unknown storage: %s", mdv_uuid_to_str(storage_uuid).ptr);
            }
            else
                MDV_LOGE("Unknown node: %s", mdv_uuid_to_str(peer_uuid).ptr);
*/
        }
        else
            MDV_LOGE("Missing data in sync message");
    }

    mdv_rollback(rollbacker);

    return MDV_OK;
}


mdv_errno mdv_datasync_update_routes(mdv_datasync *datasync)
{
    if (!mdv_datasync_is_active(datasync))
        return MDV_FAILED;
/* TODO
    mdv_vector *routes = mdv_routes_find(datasync->tracker);

    if (!routes)
    {
        MDV_LOGE("No memorty for routes");
        return MDV_NO_MEM;
    }

    if (mdv_mutex_lock(&datasync->mutex) == MDV_OK)
    {
        mdv_vector_release(datasync->routes);

        datasync->routes = routes;

        MDV_LOGD("New routes count=%zu", mdv_vector_size(routes));

        for(size_t i = 0; i < mdv_vector_size(routes); ++i)
        {
            MDV_LOGD("Route[%zu]: %u", i, *(uint32_t*)mdv_vector_at(routes, i));
        }

        mdv_mutex_unlock(&datasync->mutex);

        return MDV_OK;
    }
*/
    return MDV_FAILED;
}
#endif

