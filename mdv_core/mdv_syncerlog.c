#include "mdv_syncerlog.h"
#include "event/mdv_evt_types.h"
#include "event/mdv_evt_trlog.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_mutex.h>
#include <mdv_hashmap.h>
#include <mdv_rollbacker.h>
#include <stdatomic.h>


// TODO: Restart synchronization by timeout


/// Data synchronizer with specific node
struct mdv_syncerlog
{
    atomic_uint             rc;             ///< References counter
    mdv_uuid                uuid;           ///< Current node UUID
    mdv_uuid                peer;           ///< Global unique identifier for peer
    mdv_uuid                trlog;          ///< Global unique identifier for transaction log
    atomic_size_t           requests;       ///< TR log synchronization requests counter
    atomic_uint_fast64_t    scheduled;      ///< The last log record identifier scheduled for synchronization
    mdv_ebus               *ebus;           ///< Event bus
    mdv_jobber             *jobber;         ///< Jobs scheduler
};


static mdv_trlog * mdv_syncerlog_get(mdv_syncerlog *syncerlog)
{
    mdv_evt_trlog *evt = mdv_evt_trlog_create(&syncerlog->trlog);

    if (!evt)
    {
        MDV_LOGE("Transaction log request failed. No memory.");
        return 0;
    }

    if (mdv_ebus_publish(syncerlog->ebus, &evt->base, MDV_EVT_SYNC) != MDV_OK)
        MDV_LOGE("Transaction log request failed");

    mdv_trlog *trlog = mdv_trlog_retain(evt->trlog);

    mdv_evt_trlog_release(evt);

    return trlog;
}


static bool mdv_syncerlog_is_empty(mdv_syncerlog *syncerlog)
{
    mdv_trlog *trlog = mdv_syncerlog_get(syncerlog);

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

    int requests = 0;

    if (atomic_fetch_add_explicit(&syncerlog->requests, 1, memory_order_relaxed))
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
    size_t   requests  = atomic_load_explicit(&syncerlog->requests, memory_order_relaxed);
    uint64_t scheduled,
             trlog_top;

    do
    {
        scheduled = atomic_load_explicit(&syncerlog->scheduled, memory_order_relaxed);
        trlog_top = mdv_trlog_top(trlog);
    }
    while (!atomic_compare_exchange_strong(&syncerlog->requests, &requests, 0));

    if (scheduled >= trlog_top)
        return MDV_OK;

    if (!atomic_compare_exchange_strong(&syncerlog->scheduled, &scheduled, trlog_top))
        return MDV_FAILED;

    // Only one request should reach this point

    MDV_LOGE("TODOOOOOOOOOOOOOOOOOOO");

    return MDV_OK;
}


static mdv_errno mdv_syncerlog_evt_trlog_state(void *arg, mdv_event *event)
{
    mdv_syncerlog *syncerlog = arg;
    mdv_evt_trlog_state *state = (mdv_evt_trlog_state *)event;

    if(mdv_uuid_cmp(&syncerlog->uuid, &state->to) != 0
      || mdv_uuid_cmp(&syncerlog->trlog, &state->trlog) != 0)
        return MDV_OK;

    mdv_errno err = MDV_FAILED;

    mdv_trlog *trlog = mdv_syncerlog_get(syncerlog);

    if (trlog)
    {
        err = mdv_syncerlog_schedule(syncerlog, trlog, state->top);
        mdv_trlog_release(trlog);
    }

    return err;
}


static const mdv_event_handler_type mdv_syncerlog_handlers[] =
{
    { MDV_EVT_TRLOG_CHANGED,    mdv_syncerlog_evt_changed },
    { MDV_EVT_TRLOG_STATE,      mdv_syncerlog_evt_trlog_state },
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
    mdv_syncerlog_cancel(syncerlog);

    mdv_ebus_unsubscribe_all(syncerlog->ebus,
                             syncerlog,
                             mdv_syncerlog_handlers,
                             sizeof mdv_syncerlog_handlers / sizeof *mdv_syncerlog_handlers);

    mdv_ebus_release(syncerlog->ebus);
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


void mdv_syncerlog_cancel(mdv_syncerlog *syncerlog)
{}

