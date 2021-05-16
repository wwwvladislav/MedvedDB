#include "mdv_committer.h"
#include "event/mdv_evt_types.h"
#include "event/mdv_evt_topology.h"
#include "event/mdv_evt_trlog.h"
#include <stdatomic.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <mdv_eventfd.h>
#include <mdv_uuid.h>
#include <mdv_mutex.h>
#include <mdv_hashmap.h>
#include <mdv_threads.h>
#include <mdv_safeptr.h>


/// Data committer
struct mdv_committer
{
    atomic_uint     rc;             ///< References counter
    mdv_ebus       *ebus;           ///< Event bus
    mdv_jobber     *jobber;         ///< Jobs scheduler
    atomic_size_t   active_jobs;    ///< Active jobs counter
    mdv_safeptr    *topology;       ///< Current network topology
    mdv_mutex       mutex;          ///< Mutex for TR log synchronizers guard
    mdv_hashmap    *committers;     ///< TR log committers (hashmap<mdv_log_committer>)
};


typedef struct
{
    mdv_uuid        trlog;          ///< TR log uuid
    atomic_size_t   requests;       ///< Commit requests counter
} mdv_log_committer;


typedef struct mdv_committer_context
{
    mdv_committer     *committer;       ///< Data committer
    mdv_log_committer *log_committer;   ///< TR log committer
} mdv_committer_context;


typedef mdv_job(mdv_committer_context)     mdv_committer_job;


static void mdv_committer_fn(mdv_job_base *job)
{
    mdv_committer_context *ctx      = (mdv_committer_context *)job->data;
    mdv_committer         *committer = ctx->committer;

    size_t requests = atomic_load_explicit(&ctx->log_committer->requests, memory_order_relaxed);

    do
    {
        mdv_evt_trlog_apply *apply = mdv_evt_trlog_apply_create(&ctx->log_committer->trlog);

        if (apply)
        {
            if (mdv_ebus_publish(committer->ebus, &apply->base, MDV_EVT_SYNC) != MDV_OK)
                MDV_LOGE("Transaction log was not applied");
            mdv_evt_trlog_apply_release(apply);
        }
        else
        {
            MDV_LOGE("No memory for 'TR log apply' message");
            break;
        }
    }
    while (!atomic_compare_exchange_strong(&ctx->log_committer->requests, &requests, 0));
}


static void mdv_committer_finalize(mdv_job_base *job)
{
    mdv_committer_context *ctx = (mdv_committer_context *)job->data;
    mdv_committer         *committer = ctx->committer;
    mdv_committer_release(committer);
    atomic_fetch_sub_explicit(&committer->active_jobs, 1, memory_order_relaxed);
    mdv_free(job);
}


static mdv_errno mdv_committer_job_emit(mdv_committer *committer, mdv_log_committer *log_committer)
{
    mdv_committer_job *job = mdv_alloc(sizeof(mdv_committer_job));

    if (!job)
    {
        MDV_LOGE("No memory for data committer job");
        return MDV_NO_MEM;
    }

    job->fn                 = mdv_committer_fn;
    job->finalize           = mdv_committer_finalize;
    job->data.committer     = mdv_committer_retain(committer);
    job->data.log_committer = log_committer;

    mdv_errno err = mdv_jobber_push(committer->jobber, (mdv_job_base*)job);

    if (err != MDV_OK)
    {
        MDV_LOGE("Data committer job failed");
        mdv_committer_release(committer);
        mdv_free(job);
    }
    else
        atomic_fetch_add_explicit(&committer->active_jobs, 1, memory_order_relaxed);

    return err;
}


static mdv_log_committer * mdv_committer4trlog(mdv_committer *committer, mdv_uuid const *trlog)
{
    mdv_log_committer *lc = 0;

    if (mdv_mutex_lock(&committer->mutex) == MDV_OK)
    {
        lc = mdv_hashmap_find(committer->committers, trlog);

        if (!lc)
        {
            mdv_log_committer new_lc =
            {
                .trlog = *trlog
            };

            lc = mdv_hashmap_insert(committer->committers, &new_lc, sizeof new_lc);

            if (lc)
                atomic_init(&lc->requests, 0);
            else
                MDV_LOGE("TR log committer registration failed");
        }

        mdv_mutex_unlock(&committer->mutex);
    }

    return lc;
}


static mdv_errno mdv_committer_start(mdv_committer *committer, mdv_uuid const *trlog)
{
    mdv_log_committer *lc = mdv_committer4trlog(committer, trlog);

    mdv_errno err = MDV_FAILED;

    if(lc)
    {
        if (atomic_fetch_add_explicit(&lc->requests, 1, memory_order_relaxed) == 0)
            err = mdv_committer_job_emit(committer, lc);
    }

    return err;
}


static mdv_errno mdv_committer_evt_topology(void *arg, mdv_event *event)
{
    mdv_committer *committer = arg;
    mdv_evt_topology *topo = (mdv_evt_topology *)event;
    return mdv_safeptr_set(committer->topology, topo->topology);
}


static mdv_errno mdv_committer_evt_trlog_changed(void *arg, mdv_event *event)
{
    mdv_committer *committer = arg;
    mdv_evt_trlog_changed *evt = (mdv_evt_trlog_changed *)event;
    return mdv_committer_start(committer, &evt->trlog);
}


static const mdv_event_handler_type mdv_committer_handlers[] =
{
    { MDV_EVT_TOPOLOGY,         mdv_committer_evt_topology },
    { MDV_EVT_TRLOG_CHANGED,    mdv_committer_evt_trlog_changed },
};


mdv_committer * mdv_committer_create(mdv_ebus *ebus, mdv_jobber_config const *jconfig, mdv_topology *topology)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(6);

    mdv_committer *committer = mdv_alloc(sizeof(mdv_committer));

    if (!committer)
    {
        MDV_LOGE("No memory for new committer");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, committer);

    atomic_init(&committer->rc, 1);
    atomic_init(&committer->active_jobs, 0);

    committer->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, committer->ebus);

    committer->topology = mdv_safeptr_create(
                                topology,
                                (mdv_safeptr_retain_fn)mdv_topology_retain,
                                (mdv_safeptr_release_fn)mdv_topology_release);

    if (!committer->topology)
    {
        MDV_LOGE("Safe pointer creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_safeptr_free, committer->topology);

    if (mdv_mutex_create(&committer->mutex) != MDV_OK)
    {
        MDV_LOGE("Mutex for TR log committers not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &committer->mutex);

    committer->committers = mdv_hashmap_create(mdv_log_committer,
                                               trlog,
                                               4,
                                               mdv_uuid_hash,
                                               mdv_uuid_cmp);
    if (!committer->committers)
    {
        MDV_LOGE("There is no memory for trlog committers hashmap");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, committer->committers);

    committer->jobber = mdv_jobber_create(jconfig);

    if (!committer->jobber)
    {
        MDV_LOGE("Jobs scheduler creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_jobber_release, committer->jobber);

    if (mdv_ebus_subscribe_all(committer->ebus,
                               committer,
                               mdv_committer_handlers,
                               sizeof mdv_committer_handlers / sizeof *mdv_committer_handlers) != MDV_OK)
    {
        MDV_LOGE("Ebus subscription failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_free(rollbacker);

    return committer;
}


static void mdv_committer_free(mdv_committer *committer)
{
    mdv_ebus_unsubscribe_all(committer->ebus,
                             committer,
                             mdv_committer_handlers,
                             sizeof mdv_committer_handlers / sizeof *mdv_committer_handlers);

    mdv_ebus_release(committer->ebus);

    while(atomic_load_explicit(&committer->active_jobs, memory_order_relaxed) > 0)
        mdv_sleep(100);

    mdv_jobber_release(committer->jobber);

    mdv_safeptr_free(committer->topology);

    mdv_mutex_free(&committer->mutex);

    mdv_hashmap_release(committer->committers);

    memset(committer, 0, sizeof(*committer));

    mdv_free(committer);
}


mdv_committer * mdv_committer_retain(mdv_committer *committer)
{
    atomic_fetch_add_explicit(&committer->rc, 1, memory_order_acquire);
    return committer;
}


uint32_t mdv_committer_release(mdv_committer *committer)
{
    if (!committer)
        return 0;

    uint32_t rc = atomic_fetch_sub_explicit(&committer->rc, 1, memory_order_release) - 1;

    if (!rc)
        mdv_committer_free(committer);

    return rc;
}
