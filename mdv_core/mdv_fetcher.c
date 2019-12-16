#include "mdv_fetcher.h"
#include "event/mdv_evt_types.h"
#include "event/mdv_evt_rowdata.h"
#include "event/mdv_evt_status.h"
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


static mdv_errno mdv_syncer_evt_rowdata_fetch(void *arg, mdv_event *event)
{
    mdv_fetcher *fetcher = arg;
    mdv_evt_rowdata_fetch *fetch = (mdv_evt_rowdata_fetch *)event;

    mdv_errno err = MDV_FAILED;


    MDV_LOGE("FETCH!!! TODO!!!");

    return err;
}


static const mdv_event_handler_type mdv_fetcher_handlers[] =
{
    { MDV_EVT_ROWDATA_FETCH,    mdv_syncer_evt_rowdata_fetch },
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
