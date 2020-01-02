#include "mdv_fetcher.h"
#include "event/mdv_evt_types.h"
#include "event/mdv_evt_rowdata.h"
#include "event/mdv_evt_status.h"
#include <mdv_table.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <stdatomic.h>


/// Data fetcher
struct mdv_fetcher
{
    atomic_uint     rc;             ///< References counter
    mdv_ebus       *ebus;           ///< Event bus
    mdv_jobber     *jobber;         ///< Jobs scheduler
    atomic_size_t   active_jobs;    ///< Active jobs counter
};


typedef struct mdv_fetcher_context
{
    mdv_fetcher    *fetcher;        ///< Data fetcher
    mdv_uuid        session;        ///< Session identifier
    uint16_t        request_id;     ///< Request identifier (used to associate requests and responses)
    mdv_uuid        table;          ///< Table identifier
    bool            first;          ///< Flag indicates that the first row should be fetched
    mdv_objid       rowid;          ///< First row identifier to be fetched
    uint32_t        count;          ///< Batch size to be fetched
    mdv_bitset     *fields;         ///< Fields mask to be fetched
} mdv_fetcher_context;


typedef mdv_job(mdv_fetcher_context)     mdv_fetcher_job;


static mdv_rowdata * mdv_fetcher_rowdata(mdv_fetcher *fetcher, mdv_uuid const *table_id)
{
    mdv_rowdata *rowdata = 0;

    mdv_evt_rowdata *evt = mdv_evt_rowdata_create(table_id);

    if (evt)
    {
        if (mdv_ebus_publish(fetcher->ebus, &evt->base, MDV_EVT_SYNC) == MDV_OK)
        {
            rowdata = evt->rowdata;
            evt->rowdata = 0;
        }

        mdv_evt_rowdata_release(evt);
    }

    return rowdata;
}


static void mdv_fetcher_fn(mdv_job_base *job)
{
    mdv_fetcher_context *ctx     = (mdv_fetcher_context *)job->data;
    mdv_fetcher         *fetcher = ctx->fetcher;

    mdv_rowdata *rowdata = mdv_fetcher_rowdata(fetcher, &ctx->table);

    mdv_errno err = MDV_FAILED;

    if (rowdata)
    {
        // TODO
        MDV_LOGE("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
        mdv_rowdata_release(rowdata);
    }

    if (err != MDV_OK)
    {
        mdv_evt_status *evt = mdv_evt_status_create(
                                        &ctx->session,
                                        ctx->request_id,
                                        err,
                                        "");

        if (evt)
        {
            err = mdv_ebus_publish(fetcher->ebus, &evt->base, MDV_EVT_SYNC);
            mdv_evt_status_release(evt);
        }
    }
}


static void mdv_fetcher_finalize(mdv_job_base *job)
{
    mdv_fetcher_context *ctx     = (mdv_fetcher_context *)job->data;
    mdv_fetcher         *fetcher = ctx->fetcher;
    mdv_fetcher_release(fetcher);
    mdv_bitset_release(ctx->fields);
    atomic_fetch_sub_explicit(&fetcher->active_jobs, 1, memory_order_relaxed);
    mdv_free(job, "fetcher_job");
}


static mdv_errno mdv_fetcher_job_emit(mdv_fetcher *fetcher, mdv_evt_rowdata_fetch const *fetch)
{
    mdv_fetcher_job *job = mdv_alloc(sizeof(mdv_fetcher_job), "fetcher_job");

    if (!job)
    {
        MDV_LOGE("No memory for data fetcher job");
        return MDV_NO_MEM;
    }

    job->fn                 = mdv_fetcher_fn;
    job->finalize           = mdv_fetcher_finalize;
    job->data.fetcher       = mdv_fetcher_retain(fetcher);
    job->data.session       = fetch->session;
    job->data.request_id    = fetch->request_id;
    job->data.table         = fetch->table;
    job->data.first         = fetch->first;
    job->data.rowid         = fetch->rowid;
    job->data.count         = fetch->count;
    job->data.fields        = mdv_bitset_retain(fetch->fields);

    mdv_errno err = mdv_jobber_push(fetcher->jobber, (mdv_job_base*)job);

    if (err != MDV_OK)
    {
        MDV_LOGE("Data fetch job failed");
        mdv_fetcher_release(fetcher);
        mdv_free(job, "fetcher_job");
    }
    else
        atomic_fetch_add_explicit(&fetcher->active_jobs, 1, memory_order_relaxed);

    return err;
}


static mdv_errno mdv_fetcher_evt_rowdata_fetch(void *arg, mdv_event *event)
{
    mdv_fetcher *fetcher = arg;
    mdv_evt_rowdata_fetch *fetch = (mdv_evt_rowdata_fetch *)event;

    mdv_errno err = mdv_fetcher_job_emit(fetcher, fetch);

    if (err != MDV_OK)
    {
        mdv_evt_status *evt = mdv_evt_status_create(
                                        &fetch->session,
                                        fetch->request_id,
                                        err,
                                        "");

        if (evt)
        {
            err = mdv_ebus_publish(fetcher->ebus, &evt->base, MDV_EVT_SYNC);
            mdv_evt_status_release(evt);
        }
    }

    return err;
}


static const mdv_event_handler_type mdv_fetcher_handlers[] =
{
    { MDV_EVT_ROWDATA_FETCH,    mdv_fetcher_evt_rowdata_fetch },
};


mdv_fetcher * mdv_fetcher_create(mdv_ebus *ebus, mdv_jobber_config const *jconfig)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_fetcher *fetcher = mdv_alloc(sizeof(mdv_fetcher), "fetcher");

    if (!fetcher)
    {
        MDV_LOGE("No memory for new fetcher");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, fetcher, "fetcher");

    atomic_init(&fetcher->rc, 1);
    atomic_init(&fetcher->active_jobs, 0);

    fetcher->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, fetcher->ebus);

    fetcher->jobber = mdv_jobber_create(jconfig);

    if (!fetcher->jobber)
    {
        MDV_LOGE("Jobs scheduler creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_jobber_release, fetcher->jobber);

    if (mdv_ebus_subscribe_all(fetcher->ebus,
                               fetcher,
                               mdv_fetcher_handlers,
                               sizeof mdv_fetcher_handlers / sizeof *mdv_fetcher_handlers) != MDV_OK)
    {
        MDV_LOGE("Ebus subscription failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_free(rollbacker);

    return fetcher;
}


mdv_fetcher * mdv_fetcher_retain(mdv_fetcher *fetcher)
{
    atomic_fetch_add_explicit(&fetcher->rc, 1, memory_order_acquire);
    return fetcher;
}


static void mdv_fetcher_free(mdv_fetcher *fetcher)
{
    mdv_ebus_unsubscribe_all(fetcher->ebus,
                             fetcher,
                             mdv_fetcher_handlers,
                             sizeof mdv_fetcher_handlers / sizeof *mdv_fetcher_handlers);

    mdv_ebus_release(fetcher->ebus);

    while(atomic_load_explicit(&fetcher->active_jobs, memory_order_relaxed) > 0)
        mdv_sleep(100);

    mdv_jobber_release(fetcher->jobber);

    memset(fetcher, 0, sizeof(*fetcher));
    mdv_free(fetcher, "fetcher");
}


uint32_t mdv_fetcher_release(mdv_fetcher *fetcher)
{
    if (!fetcher)
        return 0;

    uint32_t rc = atomic_fetch_sub_explicit(&fetcher->rc, 1, memory_order_release) - 1;

    if (!rc)
        mdv_fetcher_free(fetcher);

    return rc;
}
