#include "mdv_syncerlog.h"
#include "event/mdv_evt_types.h"
#include "event/mdv_evt_trlog.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_mutex.h>
#include <mdv_hashmap.h>
#include <mdv_rollbacker.h>
#include <stdatomic.h>
#include <assert.h>


/// Data synchronizer with specific node
struct mdv_syncerlog
{
    atomic_uint             rc;             ///< References counter
    mdv_uuid                uuid;           ///< Current node UUID
    mdv_uuid                peer;           ///< Global unique identifier for peer
    mdv_uuid                trlog;          ///< Global unique identifier for transaction log
    atomic_size_t           requests;       ///< TR log synchronization requests counter
    atomic_uint_fast64_t    scheduled;      ///< The last log record identifier scheduled for synchronization
    atomic_size_t           active_jobs;    ///< Active jobs counter
    mdv_ebus               *ebus;           ///< Event bus
    mdv_jobber             *jobber;         ///< Jobs scheduler
};


static mdv_trlog * mdv_syncerlog_get(mdv_syncerlog *syncerlog, bool create)
{
    mdv_evt_trlog *evt = mdv_evt_trlog_create(&syncerlog->trlog, create);

    if (!evt)
    {
        MDV_LOGE("Transaction log request failed. No memory.");
        return 0;
    }

    mdv_errno err = mdv_ebus_publish(syncerlog->ebus, &evt->base, MDV_EVT_SYNC);

    if (create && err != MDV_OK)
        MDV_LOGE("Transaction log request failed with error %d", err);

    mdv_trlog *trlog = mdv_trlog_retain(evt->trlog);

    mdv_evt_trlog_release(evt);

    return trlog;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Job for data sending
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct mdv_syncerlog_data_send_context
{
    mdv_syncerlog  *syncer;                 ///< Data synchronizer
    mdv_trlog      *trlog;                  ///< TR log for synchronization
    uint64_t        range[2];               ///< TR log entries range for synchronization
} mdv_syncerlog_data_send_context;


typedef mdv_job(mdv_syncerlog_data_send_context)    mdv_syncerlog_data_send_job;


static void mdv_syncerlog_data_send_fn(mdv_job_base *job)
{
    mdv_syncerlog_data_send_context *ctx = (mdv_syncerlog_data_send_context *)job->data;
    mdv_syncerlog                   *syncer = ctx->syncer;

    mdv_list/*<mdv_trlog_data>*/ rows = {};

    size_t const count = mdv_trlog_range_read(
                            ctx->trlog,
                            ctx->range[0],
                            ctx->range[1] + 1,
                            &rows);

    if (count)
    {
        mdv_evt_trlog_data *evt = mdv_evt_trlog_data_create(
                                        mdv_trlog_uuid(ctx->trlog),
                                        &syncer->uuid,
                                        &syncer->peer,
                                        &rows,
                                        (uint32_t)count);

        if (evt)
        {
            if (mdv_ebus_publish(syncer->ebus, &evt->base, MDV_EVT_SYNC) != MDV_OK)
                MDV_LOGE("Transaction synchronization failed");
            mdv_evt_trlog_data_release(evt);
        }
        else
            MDV_LOGE("Transaction synchronization failed");

        mdv_list_clear(&rows);
    }
    else
        MDV_LOGE("Transaction synchronization failed");
}


static void mdv_syncerlog_data_send_finalize(mdv_job_base *job)
{
    mdv_syncerlog_data_send_context *ctx = (mdv_syncerlog_data_send_context *)job->data;
    mdv_syncerlog                   *syncer = ctx->syncer;
    mdv_trlog_release(ctx->trlog);
    atomic_fetch_sub_explicit(&syncer->active_jobs, 1, memory_order_relaxed);
    mdv_syncerlog_release(syncer);
    mdv_free(job, "syncer_data_send_job");
}


static mdv_errno mdv_syncerlog_data_send_job_emit(mdv_syncerlog *syncerlog, mdv_trlog *trlog, uint64_t lrange, uint64_t rrange)
{
    mdv_syncerlog_data_send_job *job = mdv_alloc(sizeof(mdv_syncerlog_data_send_job), "syncer_data_send_job");

    if (!job)
    {
        MDV_LOGE("No memory for data synchronization job");
        return MDV_NO_MEM;
    }

    job->fn             = mdv_syncerlog_data_send_fn;
    job->finalize       = mdv_syncerlog_data_send_finalize;
    job->data.syncer    = mdv_syncerlog_retain(syncerlog);
    job->data.trlog     = mdv_trlog_retain(trlog);
    job->data.range[0]  = lrange;
    job->data.range[1]  = rrange;

    mdv_errno err = mdv_jobber_push(syncerlog->jobber, (mdv_job_base*)job);

    if (err != MDV_OK)
    {
        MDV_LOGE("Data synchronization job failed");
        mdv_trlog_release(trlog);
        mdv_syncerlog_release(syncerlog);
        mdv_free(job, "syncer_data_send_job");
    }
    else
        atomic_fetch_add_explicit(&syncerlog->active_jobs, 1, memory_order_relaxed);

    return err;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Job for data saving
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct mdv_syncerlog_data_save_context
{
    mdv_syncerlog  *syncer;                 ///< Data synchronizer
    mdv_list        rows;                   ///< transaction log data (list<mdv_trlog_data>)
    uint32_t        count;                  ///< log records count
} mdv_syncerlog_data_save_context;


typedef mdv_job(mdv_syncerlog_data_save_context)    mdv_syncerlog_data_save_job;


static void mdv_syncerlog_data_save_fn(mdv_job_base *job)
{
    mdv_syncerlog_data_save_context *ctx = (mdv_syncerlog_data_save_context *)job->data;
    mdv_syncerlog                   *syncer = ctx->syncer;

    mdv_trlog *trlog = mdv_syncerlog_get(syncer, true);

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

    mdv_evt_trlog_state *evt = mdv_evt_trlog_state_create(&syncer->trlog, &syncer->peer, &syncer->uuid, trlog_top);

    if (evt)
    {
        if (mdv_ebus_publish(syncer->ebus, &evt->base, MDV_EVT_DEFAULT) != MDV_OK)
            MDV_LOGE("Transaction synchronization failed. Rows count: %u", ctx->count);
        mdv_evt_trlog_state_release(evt);
    }
}


static void mdv_syncerlog_data_save_finalize(mdv_job_base *job)
{
    mdv_syncerlog_data_save_context *ctx = (mdv_syncerlog_data_save_context *)job->data;
    mdv_syncerlog                   *syncer = ctx->syncer;
    mdv_list_clear(&ctx->rows);
    atomic_fetch_sub_explicit(&syncer->active_jobs, 1, memory_order_relaxed);
    mdv_syncerlog_release(syncer);
    mdv_free(job, "syncer_data_save_job");
}


static mdv_errno mdv_syncerlog_data_save_job_emit(mdv_syncerlog *syncerlog, mdv_list *rows, uint32_t count)
{
    mdv_syncerlog_data_save_job *job = mdv_alloc(sizeof(mdv_syncerlog_data_save_job), "syncer_data_save_job");

    if (!job)
    {
        MDV_LOGE("No memory for data synchronization job");
        return MDV_NO_MEM;
    }

    job->fn             = mdv_syncerlog_data_save_fn;
    job->finalize       = mdv_syncerlog_data_save_finalize;
    job->data.syncer    = mdv_syncerlog_retain(syncerlog);
    job->data.rows      = mdv_list_move(rows);
    job->data.count     = count;

    mdv_errno err = mdv_jobber_push(syncerlog->jobber, (mdv_job_base*)job);

    if (err != MDV_OK)
    {
        MDV_LOGE("Data synchronization job failed");
        mdv_syncerlog_release(syncerlog);
        *rows = mdv_list_move(&job->data.rows);
        mdv_free(job, "syncer_data_save_job");
    }
    else
        atomic_fetch_add_explicit(&syncerlog->active_jobs, 1, memory_order_relaxed);

    return err;
}


static bool mdv_syncerlog_is_empty(mdv_syncerlog *syncerlog)
{
    mdv_trlog *trlog = mdv_syncerlog_get(syncerlog, false);

    if(!trlog)
        return true;

    bool const trlog_is_empty = mdv_trlog_top(trlog) == 0;

    mdv_trlog_release(trlog);

    return trlog_is_empty;
}


static mdv_errno mdv_syncerlog_start(mdv_syncerlog *syncerlog)
{
     if(mdv_syncerlog_is_empty(syncerlog))
        return MDV_OK;

    if (atomic_fetch_add_explicit(&syncerlog->requests, 1, memory_order_relaxed) != 0)
        return MDV_OK;

    mdv_errno err = MDV_OK;

    mdv_evt_trlog_sync *sync = mdv_evt_trlog_sync_create(
                                    &syncerlog->trlog,
                                    &syncerlog->uuid,
                                    &syncerlog->peer);

    if (sync)
    {
        err = mdv_ebus_publish(syncerlog->ebus, &sync->base, MDV_EVT_SYNC);
        if (err != MDV_OK)
            MDV_LOGE("Transaction synchronization failed");
        mdv_evt_trlog_sync_release(sync);
    }
    else
    {
        err = MDV_NO_MEM;
        MDV_LOGE("Transaction synchronization failed. No memory.");
    }

    if (err != MDV_OK)
        atomic_init(&syncerlog->requests, 0);

    return err;
}

static mdv_errno mdv_syncerlog_evt_changed(void *arg, mdv_event *event)
{
    mdv_syncerlog *syncerlog = arg;
    mdv_evt_trlog_changed *evt = (mdv_evt_trlog_changed *)event;
    return mdv_syncerlog_start(syncerlog);
}


static mdv_errno mdv_syncerlog_schedule(mdv_syncerlog *syncerlog, mdv_trlog *trlog, uint64_t pos)
{
    size_t   requests = atomic_load_explicit(&syncerlog->requests, memory_order_relaxed);
    uint64_t scheduled = atomic_load_explicit(&syncerlog->scheduled, memory_order_relaxed),
             trlog_top;

    mdv_errno err = MDV_OK;

    do
    {
        do
        {
            trlog_top = mdv_trlog_top(trlog);

            if (scheduled >= trlog_top)
                return MDV_OK;

        } while (!atomic_compare_exchange_weak(&syncerlog->scheduled, &scheduled, trlog_top));

        assert(scheduled < trlog_top);

        err = mdv_syncerlog_data_send_job_emit(syncerlog, trlog, scheduled, trlog_top);

    } while(!atomic_compare_exchange_weak(&syncerlog->requests, &requests, 0));

    return err;
}


static mdv_errno mdv_syncerlog_evt_trlog_state(void *arg, mdv_event *event)
{
    mdv_syncerlog *syncerlog = arg;
    mdv_evt_trlog_state *state = (mdv_evt_trlog_state *)event;

    if(mdv_uuid_cmp(&syncerlog->uuid, &state->to) != 0
      || mdv_uuid_cmp(&syncerlog->trlog, &state->trlog) != 0)
        return MDV_OK;

    mdv_errno err = MDV_FAILED;

    mdv_trlog *trlog = mdv_syncerlog_get(syncerlog, false);

    if (trlog)
    {
        err = mdv_syncerlog_schedule(syncerlog, trlog, state->top);
        mdv_trlog_release(trlog);
    }

    return err;
}


static mdv_errno mdv_syncerlog_evt_trlog_data(void *arg, mdv_event *event)
{
    mdv_syncerlog *syncerlog = arg;
    mdv_evt_trlog_data *data = (mdv_evt_trlog_data *)event;

    if(mdv_uuid_cmp(&syncerlog->uuid, &data->to) != 0
      || mdv_uuid_cmp(&syncerlog->trlog, &data->trlog) != 0)
        return MDV_OK;

    mdv_errno err = mdv_syncerlog_data_save_job_emit(syncerlog, &data->rows, data->count);

    return err;
}


static const mdv_event_handler_type mdv_syncerlog_handlers[] =
{
    { MDV_EVT_TRLOG_CHANGED,    mdv_syncerlog_evt_changed },
    { MDV_EVT_TRLOG_STATE,      mdv_syncerlog_evt_trlog_state },
    { MDV_EVT_TRLOG_DATA,       mdv_syncerlog_evt_trlog_data },
};


mdv_syncerlog * mdv_syncerlog_create(mdv_uuid const *uuid, mdv_uuid const *peer, mdv_uuid const *trlog, mdv_ebus *ebus, mdv_jobber *jobber)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_syncerlog *syncerlog = mdv_alloc(sizeof(mdv_syncerlog), "syncerlog");

    if (!syncerlog)
    {
        MDV_LOGE("No memory for new syncerlog");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, syncerlog, "syncerlog");

    atomic_init(&syncerlog->rc, 1);
    atomic_init(&syncerlog->requests, 0);
    atomic_init(&syncerlog->scheduled, 0);
    atomic_init(&syncerlog->active_jobs, 0);

    syncerlog->uuid = *uuid;
    syncerlog->peer = *peer;
    syncerlog->trlog = *trlog;

    syncerlog->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, syncerlog->ebus);

    syncerlog->jobber = mdv_jobber_retain(jobber);

    mdv_rollbacker_push(rollbacker, mdv_jobber_release, syncerlog->jobber);

    if (mdv_ebus_subscribe_all(syncerlog->ebus,
                               syncerlog,
                               mdv_syncerlog_handlers,
                               sizeof mdv_syncerlog_handlers / sizeof *mdv_syncerlog_handlers) != MDV_OK)
    {
        MDV_LOGE("Ebus subscription failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_free(rollbacker);

    mdv_syncerlog_start(syncerlog);

    return syncerlog;
}


mdv_syncerlog * mdv_syncerlog_retain(mdv_syncerlog *syncerlog)
{
    atomic_fetch_add_explicit(&syncerlog->rc, 1, memory_order_acquire);
    return syncerlog;
}


static void mdv_syncerlog_free(mdv_syncerlog *syncerlog)
{
    mdv_ebus_unsubscribe_all(syncerlog->ebus,
                             syncerlog,
                             mdv_syncerlog_handlers,
                             sizeof mdv_syncerlog_handlers / sizeof *mdv_syncerlog_handlers);

    mdv_ebus_release(syncerlog->ebus);

    while(atomic_load_explicit(&syncerlog->active_jobs, memory_order_relaxed) > 0)
        mdv_sleep(100);
    mdv_jobber_release(syncerlog->jobber);

    memset(syncerlog, 0, sizeof(*syncerlog));
    mdv_free(syncerlog, "syncerlog");
}


uint32_t mdv_syncerlog_release(mdv_syncerlog *syncerlog)
{
    if (!syncerlog)
        return 0;

    uint32_t rc = atomic_fetch_sub_explicit(&syncerlog->rc, 1, memory_order_release) - 1;

    if (!rc)
        mdv_syncerlog_free(syncerlog);

    return rc;
}

