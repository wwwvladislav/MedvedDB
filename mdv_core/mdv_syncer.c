#include "mdv_syncer.h"
#include "mdv_syncerino.h"
#include "event/mdv_evt_types.h"
#include "event/mdv_evt_topology.h"
#include "event/mdv_evt_trlog.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <mdv_eventfd.h>
#include <mdv_threads.h>
#include <mdv_safeptr.h>
#include <mdv_router.h>
#include <mdv_mutex.h>
#include <mdv_hashmap.h>
#include <stdatomic.h>


/// Data synchronizer
struct mdv_syncer
{
    atomic_uint     rc;             ///< References counter
    mdv_ebus       *ebus;           ///< Event bus
    mdv_jobber     *jobber;         ///< Jobs scheduler
    mdv_descriptor  start;          ///< Signal for starting
    mdv_thread      thread;         ///< Committer thread
    atomic_bool     active;         ///< Status
    mdv_uuid        uuid;           ///< Current node UUID
    mdv_mutex       mutex;          ///< Mutex for peers synchronizers guard
    mdv_hashmap    *peers;          ///< Current synchronizing peers (hashmap<mdv_syncer_peer>)
};


/// Synchronizing peer
typedef struct
{
    mdv_uuid        uuid;           ///< Peer UUID
    mdv_syncerino  *syncerino;      ///< Transaction logs synchronizer with specific node
} mdv_syncer_peer;


typedef struct mdv_syncer_start_context
{
    mdv_syncer     *syncer;         ///< Data synchronizer
    mdv_hashmap    *routes;         ///< Destination peers
    mdv_uuid        trlog;          ///< TR log UUID for synchronization
} mdv_syncer_start_context;


// typedef mdv_job(mdv_syncer_start_context)     mdv_syncer_start_job;


static bool mdv_syncer_is_active(mdv_syncer *syncer)
{
    return atomic_load(&syncer->active);
}


#if 0
static mdv_trlog * mdv_syncer_trlog(mdv_syncer *syncer, mdv_uuid const *uuid)
{
    mdv_evt_trlog *evt = mdv_evt_trlog_create(uuid);

    if (!evt)
    {
        MDV_LOGE("Transaction log request failed. No memory.");
        return 0;
    }

    if (mdv_ebus_publish(syncer->ebus, &evt->base, MDV_EVT_SYNC) != MDV_OK)
        MDV_LOGE("Transaction log request failed");

    mdv_trlog *trlog = mdv_trlog_retain(evt->trlog);

    mdv_evt_trlog_release(evt);

    return trlog;
}


static void mdv_syncer_start_fn(mdv_job_base *job)
{
    mdv_syncer_start_context *ctx = (mdv_syncer_start_context *)job->data;
    mdv_syncer               *syncer = ctx->syncer;

    if (!mdv_syncer_is_active(syncer))
        return;

    mdv_hashmap_foreach(ctx->routes, mdv_route, route)
    {
        mdv_evt_trlog_sync *sync = mdv_evt_trlog_sync_create(&ctx->trlog, &syncer->uuid, &route->uuid);

        if (sync)
        {
            if (mdv_ebus_publish(syncer->ebus, &sync->base, MDV_EVT_SYNC) != MDV_OK)
                MDV_LOGE("Transaction synchronization failed");
            mdv_evt_trlog_sync_release(sync);
        }
        else
            MDV_LOGE("Transaction synchronization failed. No memory.");
    }
}


static void mdv_syncer_start_finalize(mdv_job_base *job)
{
    mdv_syncer_start_context *ctx = (mdv_syncer_start_context *)job->data;
    mdv_syncer               *syncer = ctx->syncer;
    mdv_hashmap_release(ctx->routes);
    mdv_syncer_release(syncer);
    atomic_fetch_sub_explicit(&syncer->active_jobs, 1, memory_order_relaxed);
    mdv_free(job, "syncer_job");
}


static void mdv_syncer_start_job_emit(mdv_syncer *syncer, mdv_hashmap *routes, mdv_uuid const *trlog)
{
    mdv_syncer_start_job *job = mdv_alloc(sizeof(mdv_syncer_start_job), "syncer_start_job");

    if (!job)
    {
        MDV_LOGE("No memory for data synchronization job");
        return;
    }

    job->fn             = mdv_syncer_start_fn;
    job->finalize       = mdv_syncer_start_finalize;
    job->data.syncer    = mdv_syncer_retain(syncer);
    job->data.routes    = mdv_hashmap_retain(routes);
    job->data.trlog     = *trlog;

    mdv_errno err = mdv_jobber_push(syncer->jobber, (mdv_job_base*)job);

    if (err != MDV_OK)
    {
        MDV_LOGE("Data synchronization job failed");
        mdv_hashmap_release(routes);
        mdv_syncer_release(syncer);
        mdv_free(job, "syncer_job");
    }
    else
    {
        atomic_fetch_add_explicit(&syncer->active_jobs, 1, memory_order_relaxed);
    }
}


// Main data synchronization thread generates asynchronous jobs for each peer.
static void mdv_syncer_start_jobs_emit(mdv_syncer *syncer)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    do
    {
        mdv_topology *topology = mdv_safeptr_get(syncer->topology);

        if (!topology)
            break;

        mdv_rollbacker_push(rollbacker, mdv_topology_release, topology);

        mdv_vector *nodes = mdv_topology_nodes(topology);

        if (!nodes)
            break;

        mdv_rollbacker_push(rollbacker, mdv_vector_release, nodes);

        if (mdv_vector_empty(nodes))
            break;

        mdv_hashmap *routes = mdv_safeptr_get(syncer->routes);

        if (!routes)
            break;

        mdv_rollbacker_push(rollbacker, mdv_hashmap_release, routes);

        if (mdv_hashmap_empty(routes))
            break;

        mdv_vector_foreach(nodes, mdv_toponode, node)
        {
            mdv_syncer_start_job_emit(syncer, routes, &node->uuid);
        }
    }
    while(0);

    mdv_rollback(rollbacker);
}
#endif


static void * mdv_syncer_thread(void *arg)
{
    mdv_syncer *syncer = arg;

    atomic_store(&syncer->active, true);

/*
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

    if (mdv_epoll_add(epfd, syncer->start, event) != MDV_OK)
    {
        MDV_LOGE("epoll event registration failed");
        mdv_epoll_close(epfd);
        return 0;
    }

    while(mdv_syncer_is_active(syncer))
    {
        mdv_epoll_event events[1];
        uint32_t size = sizeof events / sizeof *events;

        mdv_epoll_wait(epfd, events, &size, -1);

        if (!mdv_syncer_is_active(syncer))
            break;

        if (size)
        {
            if (events[0].events & MDV_EPOLLIN)
            {
                uint64_t signal;
                size_t len = sizeof(signal);

                while(mdv_read(syncer->start, &signal, &len) == MDV_EAGAIN);

                // Synchronization start
                mdv_syncer_start_jobs_emit(syncer);
            }
        }
    }

    mdv_epoll_close(epfd);
*/
    return 0;
}


static mdv_errno mdv_syncer_topology_changed(mdv_syncer *syncer, mdv_topology *topology)
{
    mdv_errno err = MDV_OK;

    mdv_hashmap *routes = mdv_routes_find(topology, &syncer->uuid);

    if (routes)
    {
        err = mdv_mutex_lock(&syncer->mutex);

        if (err == MDV_OK)
        {
            mdv_hashmap_foreach(syncer->peers, mdv_syncer_peer, peer)
            {
                if (mdv_hashmap_find(routes, &peer->uuid))
                {
                    mdv_syncerino_cancel(peer->syncerino);
                    mdv_syncerino_release(peer->syncerino);
                    // TODO: mdv_hashmap_erase(syncer->peers);
                }
            }

            mdv_hashmap_foreach(routes, mdv_route, route)
            {
                //mdv_syncer_peer *peer = mdv_hashmap_find(syncer->peers);

                //if (peer)
                    //continue;
            }

            mdv_mutex_unlock(&syncer->mutex);
        }

        //err = err == MDV_OK
        //        ? mdv_safeptr_set(syncer->routes, routes)
        //        : err;

        // TODO: Create peers synchronizers

        mdv_hashmap_release(routes);
    }
    else
        MDV_LOGE("Routes calculation failed");

    return err;
}


static mdv_errno mdv_syncer_evt_topology(void *arg, mdv_event *event)
{
    mdv_syncer *syncer = arg;
    mdv_evt_topology *topo = (mdv_evt_topology *)event;
    return mdv_syncer_topology_changed(syncer, topo->topology);
}


#if 0
static mdv_errno mdv_syncer_evt_trlog_changed(void *arg, mdv_event *event)
{
    mdv_syncer *syncer = arg;
    (void)event;
    mdv_syncer_start(syncer);
    return MDV_OK;
}


static mdv_errno mdv_syncer_evt_trlog_sync(void *arg, mdv_event *event)
{
    mdv_syncer *syncer = arg;
    mdv_evt_trlog_sync *sync = (mdv_evt_trlog_sync *)event;

    if(mdv_uuid_cmp(&syncer->uuid, &sync->to) != 0)
        return MDV_OK;

    mdv_errno err = MDV_FAILED;

    mdv_trlog *trlog = mdv_syncer_trlog(syncer, &sync->trlog);

    mdv_evt_trlog_state *evt = mdv_evt_trlog_state_create(&sync->trlog, &sync->to, &sync->from, trlog ? mdv_trlog_top(trlog) : 0);

    if (evt)
    {
        err = mdv_ebus_publish(syncer->ebus, &evt->base, MDV_EVT_DEFAULT);
        mdv_evt_trlog_state_release(evt);
    }

    mdv_trlog_release(trlog);

    return err;
}


static mdv_errno mdv_syncer_evt_trlog_state(void *arg, mdv_event *event)
{
    mdv_syncer *syncer = arg;
    mdv_evt_trlog_state *state = (mdv_evt_trlog_state *)event;

    if(mdv_uuid_cmp(&syncer->uuid, &state->to) != 0)
        return MDV_OK;

    mdv_errno err = MDV_FAILED;

    mdv_trlog *trlog = mdv_syncer_trlog(syncer, &state->trlog);

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
#endif


static const mdv_event_handler_type mdv_syncer_handlers[] =
{
    { MDV_EVT_TOPOLOGY,         mdv_syncer_evt_topology },
//    { MDV_EVT_TRLOG_CHANGED,    mdv_syncer_evt_trlog_changed },
//    { MDV_EVT_TRLOG_SYNC,       mdv_syncer_evt_trlog_sync },
//    { MDV_EVT_TRLOG_STATE,      mdv_syncer_evt_trlog_state },
};


mdv_syncer * mdv_syncer_create(mdv_uuid const *uuid,
                                   mdv_ebus *ebus,
                                   mdv_jobber_config const *jconfig,
                                   mdv_topology *topology)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(7);

    mdv_syncer *syncer = mdv_alloc(sizeof(mdv_syncer), "syncer");

    if (!syncer)
    {
        MDV_LOGE("No memory for new syncer");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, syncer, "syncer");

    atomic_init(&syncer->rc, 1);
    atomic_init(&syncer->active, false);

    syncer->uuid = *uuid;

    syncer->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, syncer->ebus);

    if (mdv_mutex_create(&syncer->mutex) != MDV_OK)
    {
        MDV_LOGE("Mutex for TR log synchronizers not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &syncer->mutex);

    syncer->peers = mdv_hashmap_create(mdv_syncer_peer,
                                       uuid,
                                       4,
                                       mdv_uuid_hash,
                                       mdv_uuid_cmp);
    if (!syncer->peers)
    {
        MDV_LOGE("There is no memory for peers hashmap");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, syncer->peers);

    if (mdv_syncer_topology_changed(syncer, topology) != MDV_OK)
    {
        MDV_LOGE("Peers synchronizeers creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    syncer->jobber = mdv_jobber_create(jconfig);

    if (!syncer->jobber)
    {
        MDV_LOGE("Jobs scheduler creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_jobber_release, syncer->jobber);

    syncer->start = mdv_eventfd(false);

    if (syncer->start == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("Eventfd creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_eventfd_close, syncer->start);

    mdv_thread_attrs const attrs =
    {
        .stack_size = MDV_THREAD_STACK_SIZE
    };

    mdv_errno err = mdv_thread_create(&syncer->thread, &attrs, mdv_syncer_thread, syncer);

    if (err != MDV_OK)
    {
        MDV_LOGE("Thread creation failed with error %d", err);
        mdv_rollback(rollbacker);
        return 0;
    }

    while(!mdv_syncer_is_active(syncer))
        mdv_sleep(100);

    mdv_rollbacker_push(rollbacker, mdv_syncer_stop, syncer);

    if (mdv_ebus_subscribe_all(syncer->ebus,
                               syncer,
                               mdv_syncer_handlers,
                               sizeof mdv_syncer_handlers / sizeof *mdv_syncer_handlers) != MDV_OK)
    {
        MDV_LOGE("Ebus subscription failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_free(rollbacker);

    return syncer;
}


void mdv_syncer_start(mdv_syncer *syncer)
{
    uint64_t const signal = 1;
    mdv_write_all(syncer->start, &signal, sizeof signal);
}


void mdv_syncer_stop(mdv_syncer *syncer)
{
    if (mdv_syncer_is_active(syncer))
    {
        atomic_store(&syncer->active, false);

        uint64_t const signal = 1;
        mdv_write_all(syncer->start, &signal, sizeof signal);

        mdv_thread_join(syncer->thread);

        mdv_hashmap_foreach(syncer->peers, mdv_syncer_peer, peer)
            mdv_syncerino_cancel(peer->syncerino);
    }
}


static void mdv_syncer_free(mdv_syncer *syncer)
{
    mdv_syncer_stop(syncer);

    mdv_ebus_unsubscribe_all(syncer->ebus,
                             syncer,
                             mdv_syncer_handlers,
                             sizeof mdv_syncer_handlers / sizeof *mdv_syncer_handlers);

    mdv_jobber_release(syncer->jobber);
    mdv_eventfd_close(syncer->start);
    mdv_ebus_release(syncer->ebus);

    mdv_hashmap_foreach(syncer->peers, mdv_syncer_peer, peer)
        mdv_syncerino_release(peer->syncerino);
    mdv_hashmap_release(syncer->peers);

    mdv_mutex_free(&syncer->mutex);

    memset(syncer, 0, sizeof(*syncer));
    mdv_free(syncer, "syncer");
}


mdv_syncer * mdv_syncer_retain(mdv_syncer *syncer)
{
    atomic_fetch_add_explicit(&syncer->rc, 1, memory_order_acquire);
    return syncer;
}


uint32_t mdv_syncer_release(mdv_syncer *syncer)
{
    if (!syncer)
        return 0;

    uint32_t rc = atomic_fetch_sub_explicit(&syncer->rc, 1, memory_order_release) - 1;

    if (!rc)
        mdv_syncer_free(syncer);

    return rc;
}
