#include "mdv_syncer.h"
#include "mdv_syncerino.h"
#include "event/mdv_evt_types.h"
#include "event/mdv_evt_topology.h"
#include "event/mdv_evt_trlog.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <mdv_threads.h>
#include <mdv_safeptr.h>
#include <mdv_router.h>
#include <mdv_mutex.h>
#include <mdv_hashmap.h>
#include <mdv_vector.h>
#include <stdatomic.h>


/// Data synchronizer
struct mdv_syncer
{
    atomic_uint     rc;             ///< References counter
    mdv_ebus       *ebus;           ///< Event bus
    mdv_jobber     *jobber;         ///< Jobs scheduler
    mdv_uuid        uuid;           ///< Current node UUID
    mdv_mutex       mutex;          ///< Mutex for peers synchronizers guard
    mdv_hashmap    *peers;          ///< Current synchronizing peers (hashmap<mdv_syncer_peer>)
    atomic_size_t   active_jobs;    ///< Active jobs counter
};


/// Synchronizing peer
typedef struct
{
    mdv_uuid        uuid;           ///< Peer UUID
    mdv_syncerino  *syncerino;      ///< Transaction logs synchronizer with specific node
} mdv_syncer_peer;


static mdv_trlog * mdv_syncer_trlog(mdv_syncer *syncer, mdv_uuid const *uuid, bool create)
{
    mdv_evt_trlog *evt = mdv_evt_trlog_create(uuid, create);

    if (!evt)
    {
        MDV_LOGE("Transaction log request failed. No memory.");
        return 0;
    }

    mdv_ebus_publish(syncer->ebus, &evt->base, MDV_EVT_SYNC);

    mdv_trlog *trlog = mdv_trlog_retain(evt->trlog);

    mdv_evt_trlog_release(evt);

    return trlog;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Job for data saving
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct mdv_syncer_data_save_context
{
    mdv_syncer     *syncer;                 ///< Data synchronizer
    mdv_uuid        peer;                   ///< Peer UUID
    mdv_uuid        trlog;                  ///< transaction log UUID
    mdv_list        rows;                   ///< transaction log data (list<mdv_trlog_data>)
    uint32_t        count;                  ///< log records count
} mdv_syncer_data_save_context;


typedef mdv_job(mdv_syncer_data_save_context)    mdv_syncer_data_save_job;


static void mdv_syncer_data_save_fn(mdv_job_base *job)
{
    mdv_syncer_data_save_context *ctx = (mdv_syncer_data_save_context *)job->data;
    mdv_syncer                   *syncer = ctx->syncer;

    mdv_trlog *trlog = mdv_syncer_trlog(syncer, &ctx->trlog, true);

    uint64_t trlog_top = 0;

    if (trlog)
    {
        if (!mdv_trlog_add(trlog, &ctx->rows))
            MDV_LOGE("Transaction synchronization failed. Rows count: %u", ctx->count);

        trlog_top = mdv_trlog_top(trlog);

        mdv_trlog_release(trlog);
    }
    else
        MDV_LOGE("Transaction synchronization failed. Rows count: %u", ctx->count);

    mdv_evt_trlog_state *evt = mdv_evt_trlog_state_create(&ctx->trlog, &syncer->uuid, &ctx->peer, trlog_top);

    if (evt)
    {
        if (mdv_ebus_publish(syncer->ebus, &evt->base, MDV_EVT_DEFAULT) != MDV_OK)
            MDV_LOGE("Transaction synchronization failed. Rows count: %u", ctx->count);
        mdv_evt_trlog_state_release(evt);
    }
}


static void mdv_syncer_data_save_finalize(mdv_job_base *job)
{
    mdv_syncer_data_save_context *ctx = (mdv_syncer_data_save_context *)job->data;
    mdv_syncer                   *syncer = ctx->syncer;
    mdv_list_clear(&ctx->rows);
    atomic_fetch_sub_explicit(&syncer->active_jobs, 1, memory_order_relaxed);
    mdv_syncer_release(syncer);
    mdv_free(job);
}


static mdv_errno mdv_syncer_data_save_job_emit(mdv_syncer *syncer, mdv_uuid const *peer, mdv_uuid const *trlog, mdv_list *rows, uint32_t count)
{
    mdv_syncer_data_save_job *job = mdv_alloc(sizeof(mdv_syncer_data_save_job));

    if (!job)
    {
        MDV_LOGE("No memory for data synchronization job");
        return MDV_NO_MEM;
    }

    job->fn             = mdv_syncer_data_save_fn;
    job->finalize       = mdv_syncer_data_save_finalize;
    job->data.syncer    = mdv_syncer_retain(syncer);
    job->data.peer      = *peer;
    job->data.trlog     = *trlog;
    job->data.rows      = mdv_list_move(rows);
    job->data.count     = count;

    mdv_errno err = mdv_jobber_push(syncer->jobber, (mdv_job_base*)job);

    if (err != MDV_OK)
    {
        MDV_LOGE("Data synchronization job failed");
        mdv_syncer_release(syncer);
        *rows = mdv_list_move(&job->data.rows);
        mdv_free(job);
    }
    else
        atomic_fetch_add_explicit(&syncer->active_jobs, 1, memory_order_relaxed);

    return err;
}


static void mdv_syncerino_start4all_storages(mdv_syncerino *syncerino, mdv_topology *topology)
{
    mdv_vector *nodes = mdv_topology_nodes(topology);

    mdv_vector_foreach(nodes, mdv_toponode, node)
        mdv_syncerino_start(syncerino, &node->uuid);

    mdv_vector_release(nodes);
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
            // I. Disconnected peers removal
            if (!mdv_hashmap_empty(syncer->peers))
            {
                mdv_vector *removed_peers = mdv_vector_create(
                                                mdv_hashmap_size(syncer->peers),
                                                sizeof(mdv_uuid),
                                                &mdv_default_allocator);

                if (removed_peers)
                {
                    mdv_hashmap_foreach(syncer->peers, mdv_syncer_peer, peer)
                    {
                        if (!mdv_hashmap_find(routes, &peer->uuid))
                        {
                            mdv_syncerino_cancel(peer->syncerino);
                            mdv_syncerino_release(peer->syncerino);
                            mdv_vector_push_back(removed_peers, &peer->uuid);
                        }
                    }

                    mdv_vector_foreach(removed_peers, mdv_uuid, peer)
                        mdv_hashmap_erase(syncer->peers, peer);

                    mdv_vector_release(removed_peers);
                }
                else
                {
                    err = MDV_NO_MEM;
                    MDV_LOGE("No memory for removed peers");
                }
            }

            // II. New peers adding
            mdv_hashmap_foreach(routes, mdv_route, route)
            {
                mdv_syncer_peer *peer = mdv_hashmap_find(syncer->peers, &route->uuid);

                if (peer)
                {
                    mdv_syncerino_start4all_storages(peer->syncerino, topology);
                    continue;
                }

                mdv_syncer_peer syncer_peer =
                {
                    .uuid = route->uuid,
                    .syncerino = mdv_syncerino_create(&syncer->uuid, &route->uuid, syncer->ebus, syncer->jobber)
                };

                if (syncer_peer.syncerino)
                {
                    if (mdv_hashmap_insert(syncer->peers, &syncer_peer, sizeof syncer_peer))
                        mdv_syncerino_start4all_storages(syncer_peer.syncerino, topology);
                    else
                    {
                        err = MDV_FAILED;
                        mdv_syncerino_release(syncer_peer.syncerino);
                        char uuid_str[MDV_UUID_STR_LEN];
                        MDV_LOGE("Synchronizer creation for '%s' peer failed", mdv_uuid_to_str(&route->uuid, uuid_str));
                    }
                }
                else
                {
                    err = MDV_FAILED;
                    char uuid_str[MDV_UUID_STR_LEN];
                    MDV_LOGE("Synchronizer creation for '%s' peer failed", mdv_uuid_to_str(&route->uuid, uuid_str));
                }
            }

            MDV_LOGI("Number of peers synchronizers is: %u", (uint32_t)mdv_hashmap_size(syncer->peers));

            mdv_mutex_unlock(&syncer->mutex);
        }

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


static mdv_errno mdv_syncer_evt_trlog_sync(void *arg, mdv_event *event)
{
    mdv_syncer *syncer = arg;
    mdv_evt_trlog_sync *sync = (mdv_evt_trlog_sync *)event;

    if(mdv_uuid_cmp(&syncer->uuid, &sync->to) != 0)
        return MDV_OK;

    mdv_errno err = MDV_FAILED;

    mdv_trlog *trlog = mdv_syncer_trlog(syncer, &sync->trlog, false);

    mdv_evt_trlog_state *evt = mdv_evt_trlog_state_create(&sync->trlog, &sync->to, &sync->from, trlog ? mdv_trlog_top(trlog) : 0);

    if (evt)
    {
        err = mdv_ebus_publish(syncer->ebus, &evt->base, MDV_EVT_DEFAULT);
        mdv_evt_trlog_state_release(evt);
    }

    mdv_trlog_release(trlog);

    return err;
}


static mdv_errno mdv_syncer_evt_trlog_data(void *arg, mdv_event *event)
{
    mdv_syncer *syncer = arg;
    mdv_evt_trlog_data *data = (mdv_evt_trlog_data *)event;

    if(mdv_uuid_cmp(&syncer->uuid, &data->to) != 0)
        return MDV_OK;

    mdv_errno err = mdv_syncer_data_save_job_emit(syncer, &data->from, &data->trlog, &data->rows, data->count);

    return err;
}


static const mdv_event_handler_type mdv_syncer_handlers[] =
{
    { MDV_EVT_TOPOLOGY,         mdv_syncer_evt_topology },
    { MDV_EVT_TRLOG_SYNC,       mdv_syncer_evt_trlog_sync },
    { MDV_EVT_TRLOG_DATA,       mdv_syncer_evt_trlog_data },
};


mdv_syncer * mdv_syncer_create(mdv_uuid const *uuid,
                               mdv_ebus *ebus,
                               mdv_jobber_config const *jconfig,
                               mdv_topology *topology)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(7);

    mdv_syncer *syncer = mdv_alloc(sizeof(mdv_syncer));

    if (!syncer)
    {
        MDV_LOGE("No memory for new syncer");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, syncer);

    atomic_init(&syncer->rc, 1);
    atomic_init(&syncer->active_jobs, 0);

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

    mdv_rollbacker_push(rollbacker, mdv_syncer_cancel, syncer);

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


void mdv_syncer_cancel(mdv_syncer *syncer)
{
    if (mdv_mutex_lock(&syncer->mutex) == MDV_OK)
    {
        mdv_hashmap_foreach(syncer->peers, mdv_syncer_peer, peer)
            mdv_syncerino_cancel(peer->syncerino);
        mdv_mutex_unlock(&syncer->mutex);
    }
}


static void mdv_syncer_free(mdv_syncer *syncer)
{
    mdv_syncer_cancel(syncer);

    mdv_ebus_unsubscribe_all(syncer->ebus,
                             syncer,
                             mdv_syncer_handlers,
                             sizeof mdv_syncer_handlers / sizeof *mdv_syncer_handlers);

    mdv_ebus_release(syncer->ebus);

    while(atomic_load_explicit(&syncer->active_jobs, memory_order_relaxed) > 0)
        mdv_sleep(100);

    mdv_jobber_release(syncer->jobber);

    mdv_hashmap_foreach(syncer->peers, mdv_syncer_peer, peer)
        mdv_syncerino_release(peer->syncerino);
    mdv_hashmap_release(syncer->peers);

    mdv_mutex_free(&syncer->mutex);

    memset(syncer, 0, sizeof(*syncer));
    mdv_free(syncer);
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
