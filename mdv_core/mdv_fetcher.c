#include "mdv_fetcher.h"
#include "mdv_config.h"
#include "event/mdv_evt_types.h"
#include "event/mdv_evt_rowdata.h"
#include "event/mdv_evt_view.h"
#include "event/mdv_evt_table.h"
#include "event/mdv_evt_status.h"
#include "storage/mdv_view.h"
#include <mdv_table.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <mdv_hashmap.h>
#include <mdv_mutex.h>
#include <mdv_time.h>
#include <stdatomic.h>


/// Data fetcher
struct mdv_fetcher
{
    atomic_uint_fast32_t    rc;             ///< References counter
    mdv_ebus               *ebus;           ///< Event bus
    mdv_jobber             *jobber;         ///< Jobs scheduler
    atomic_size_t           active_jobs;    ///< Active jobs counter
    atomic_uint_fast32_t    idgen;          ///< Identifiers generator
    mdv_mutex               mutex;          ///< Mutex for views map guard
    mdv_hashmap            *views;          ///< Active views (hashmap<mdv_fetcher_view>)
};


typedef struct
{
    uint32_t     id;                        ///< View identifier
    mdv_view    *view;                      ///< Table view
    size_t       last_access_time;          ///< Last access time
} mdv_fetcher_view;


static size_t mdv_u32_hash(uint32_t const *v)                   { return *v; }
static int mdv_u32_cmp(uint32_t const *a, uint32_t const *b)    { return (int)*a - *b; }


typedef struct mdv_fetcher_context
{
    mdv_fetcher    *fetcher;        ///< Data fetcher
    mdv_uuid        session;        ///< Session identifier
    uint16_t        request_id;     ///< Request identifier (used to associate requests and responses)
    uint32_t        view_id;        ///< View identifier
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


static mdv_table * mdv_fetcher_table(mdv_fetcher *fetcher, mdv_uuid const *table_id)
{
    mdv_table *table = 0;

    mdv_evt_table *evt = mdv_evt_table_create(table_id);

    if (evt)
    {
        if (mdv_ebus_publish(fetcher->ebus, &evt->base, MDV_EVT_SYNC) == MDV_OK)
        {
            table = evt->table;
            evt->table = 0;
        }

        mdv_evt_table_release(evt);
    }

    return table;
}


static mdv_view * mdv_fetcher_view_find(mdv_fetcher *fetcher, uint32_t view_id)
{
    mdv_view *view = 0;

    if(mdv_mutex_lock(&fetcher->mutex) == MDV_OK)
    {
        mdv_fetcher_view *fetcher_view = mdv_hashmap_find(fetcher->views, &view_id);

        if (fetcher_view)
            view = mdv_view_retain(fetcher_view->view);

        mdv_mutex_unlock(&fetcher->mutex);
    }
    else
        MDV_LOGE("Mutex lock failed");

    return view;
}


static void mdv_fetcher_fn(mdv_job_base *job)
{
    mdv_fetcher_context *ctx     = (mdv_fetcher_context *)job->data;
    mdv_fetcher         *fetcher = ctx->fetcher;

    mdv_errno err = MDV_FAILED;
    char const *err_msg = "";

    mdv_view *view = mdv_fetcher_view_find(fetcher, ctx->view_id);

    if (view)
    {
        mdv_rowset *rowset = mdv_view_fetch(view, MDV_CONFIG.fetcher.batch_size);

        if(rowset)
        {
            // TODO:
            MDV_LOGE("OOOOOOOOOOOOOOOOO");
            mdv_rowset_release(rowset);
        }

        mdv_view_release(view);
    }
    else
    {
        err_msg = "View not found";
        MDV_LOGE("View %u not found", ctx->view_id);
    }

    if (err != MDV_OK)
    {
        mdv_evt_status *evt = mdv_evt_status_create(
                                        &ctx->session,
                                        ctx->request_id,
                                        err,
                                        err_msg);

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
    atomic_fetch_sub_explicit(&fetcher->active_jobs, 1, memory_order_relaxed);
    mdv_free(job, "fetcher_job");
}


static mdv_errno mdv_fetcher_job_emit(mdv_fetcher *fetcher, mdv_evt_view_fetch const *fetch)
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
    job->data.view_id       = fetch->view_id;

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


static mdv_errno mdv_fetcher_view_register(mdv_fetcher  *fetcher,
                                           mdv_view     *view,
                                           uint32_t     *view_id)
{
    mdv_errno err = mdv_mutex_lock(&fetcher->mutex);

    if(err == MDV_OK)
    {
        mdv_fetcher_view const reg_view =
        {
            .id = atomic_fetch_add_explicit(&fetcher->idgen, 1, memory_order_relaxed),
            .view = mdv_view_retain(view),
            .last_access_time = mdv_gettime(),
        };

        if (!mdv_hashmap_insert(fetcher->views, &reg_view, sizeof reg_view))
        {
            err = MDV_NO_MEM;
            mdv_view_release(view);
        }

        mdv_mutex_unlock(&fetcher->mutex);
    }

    return err;
}


static mdv_errno mdv_fetcher_view_create(mdv_fetcher    *fetcher,
                                         mdv_uuid const *table_id,
                                         mdv_bitset     *fields,
                                         char const     *filter,
                                         char const    **err_msg,
                                         uint32_t       *view_id)
{
    mdv_errno err = MDV_FAILED;

    mdv_table *table = mdv_fetcher_table(fetcher, table_id);

    if(table)
    {
        mdv_rowdata *rowdata = mdv_fetcher_rowdata(fetcher, table_id);

        if (rowdata)
        {
            mdv_predicate * predicate = mdv_predicate_parse(filter);

            if (predicate)
            {
                mdv_view *view = mdv_view_create(rowdata, table, fields, predicate);

                if (view)
                {
                    err = mdv_fetcher_view_register(fetcher, view, view_id);

                    if (err != MDV_OK)
                        *err_msg = "View registration failed";

                    mdv_view_release(view);
                }
                else
                    *err_msg = "View creation failed";

                mdv_predicate_release(predicate);
            }
            else
                *err_msg = "Rows filter is incorrect";

            mdv_rowdata_release(rowdata);
        }
        else
            *err_msg = "Rowdata not found";

        mdv_table_release(table);
    }
    else
        *err_msg = "Table not found";

    return err;
}


static mdv_errno mdv_fetcher_evt_select(void *arg, mdv_event *event)
{
    mdv_fetcher *fetcher = arg;
    mdv_evt_select *select = (mdv_evt_select *)event;

    uint32_t view_id = ~0u;
    char const *err_msg = "";

    mdv_errno err = mdv_fetcher_view_create(fetcher,
                                            &select->table,
                                            select->fields,
                                            select->filter,
                                            &err_msg,
                                            &view_id);

    if (err == MDV_OK)
    {
        mdv_evt_view *evt = mdv_evt_view_create(
                                        &select->session,
                                        select->request_id,
                                        view_id);

        if (evt)
        {
            err = mdv_ebus_publish(fetcher->ebus, &evt->base, MDV_EVT_SYNC);
            mdv_evt_view_release(evt);
        }
    }
    else
    {
        MDV_LOGE("Selection reqiest failed for table '%s' with error '%s'",
                    mdv_uuid_to_str(&select->table).ptr,
                    err_msg);

        mdv_evt_status *evt = mdv_evt_status_create(
                                        &select->session,
                                        select->request_id,
                                        err,
                                        err_msg);

        if (evt)
        {
            err = mdv_ebus_publish(fetcher->ebus, &evt->base, MDV_EVT_SYNC);
            mdv_evt_status_release(evt);
        }
    }

    return MDV_OK;
}


static mdv_errno mdv_fetcher_evt_view_fetch(void *arg, mdv_event *event)
{
    mdv_fetcher *fetcher = arg;
    mdv_evt_view_fetch *fetch = (mdv_evt_view_fetch *)event;

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
    { MDV_EVT_SELECT,       mdv_fetcher_evt_select },
    { MDV_EVT_VIEW_FETCH,   mdv_fetcher_evt_view_fetch },
};


mdv_fetcher * mdv_fetcher_create(mdv_ebus *ebus, mdv_jobber_config const *jconfig)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(5);

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
    atomic_init(&fetcher->idgen, 0);

    fetcher->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, fetcher->ebus);

    if (mdv_mutex_create(&fetcher->mutex) != MDV_OK)
    {
        MDV_LOGE("Mutex creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &fetcher->mutex);

    fetcher->views = mdv_hashmap_create(mdv_fetcher_view, id, 8, mdv_u32_hash, mdv_u32_cmp);

    if (!fetcher->views)
    {
        MDV_LOGE("Views map creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, fetcher->views);

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

    mdv_hashmap_foreach(fetcher->views, mdv_fetcher_view, entry)
        mdv_view_release(entry->view);

    mdv_hashmap_release(fetcher->views);

    mdv_mutex_free(&fetcher->mutex);

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
