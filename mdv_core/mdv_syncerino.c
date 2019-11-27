#include "mdv_syncerino.h"
#include "event/mdv_evt_types.h"
#include "event/mdv_evt_trlog.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_mutex.h>
#include <mdv_hashmap.h>
#include <mdv_rollbacker.h>
#include <stdatomic.h>


/// Data synchronizer with specific node
struct mdv_syncerino
{
    atomic_uint     rc;             ///< References counter
    mdv_ebus       *ebus;           ///< Event bus
    mdv_uuid        uuid;           ///< Global unique identifier for node
    mdv_mutex       mutex;          ///< Mutex for TR log synchronizers guard
    mdv_hashmap    *trlogs;         ///< Current synchronizing TR logs (hashmap<mdv_syncerino_trlog>)
};


typedef struct
{
    mdv_uuid    uuid;               ///< TR log UUID
} mdv_syncerino_trlog;


static mdv_errno mdv_syncerino_evt_trlog_changed(void *arg, mdv_event *event)
{
    mdv_syncerino *syncerino = arg;
    mdv_evt_trlog_changed *evt = (mdv_evt_trlog_changed *)event;
    return mdv_syncerino_start(syncerino, &evt->trlog);
}


static const mdv_event_handler_type mdv_syncerino_handlers[] =
{
    { MDV_EVT_TRLOG_CHANGED,    mdv_syncerino_evt_trlog_changed },
//    { MDV_EVT_TRLOG_SYNC,       mdv_syncer_evt_trlog_sync },
//    { MDV_EVT_TRLOG_STATE,      mdv_syncer_evt_trlog_state },
};


mdv_syncerino * mdv_syncerino_create(mdv_uuid const *uuid, mdv_ebus *ebus)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    mdv_syncerino *syncerino = mdv_alloc(sizeof(mdv_syncerino), "syncerino");

    if (!syncerino)
    {
        MDV_LOGE("No memory for new syncerino");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, syncerino, "syncerino");

    atomic_init(&syncerino->rc, 1);

    syncerino->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, syncerino->ebus);

    if (mdv_mutex_create(&syncerino->mutex) != MDV_OK)
    {
        MDV_LOGE("Mutex for TR log synchronizers not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &syncerino->mutex);

    syncerino->trlogs = mdv_hashmap_create(mdv_syncerino_trlog,
                                           uuid,
                                           4,
                                           mdv_uuid_hash,
                                           mdv_uuid_cmp);
    if (!syncerino->trlogs)
    {
        MDV_LOGE("There is no memory for trlogs hashmap");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, syncerino->trlogs);

    mdv_rollbacker_free(rollbacker);

    return syncerino;
}


mdv_syncerino * mdv_syncerino_retain(mdv_syncerino *syncerino)
{
    atomic_fetch_add_explicit(&syncerino->rc, 1, memory_order_acquire);
    return syncerino;
}


static void mdv_syncerino_free(mdv_syncerino *syncerino)
{
    mdv_syncerino_cancel(syncerino);
    mdv_ebus_release(syncerino->ebus);
    mdv_hashmap_release(syncerino->trlogs);
    mdv_mutex_free(&syncerino->mutex);
    memset(syncerino, 0, sizeof(*syncerino));
    mdv_free(syncerino, "syncerino");
}


uint32_t mdv_syncerino_release(mdv_syncerino *syncerino)
{
    if (!syncerino)
        return 0;

    uint32_t rc = atomic_fetch_sub_explicit(&syncerino->rc, 1, memory_order_release) - 1;

    if (!rc)
        mdv_syncerino_free(syncerino);

    return rc;
}


void mdv_syncerino_cancel(mdv_syncerino *syncerino)
{}


mdv_errno mdv_syncerino_start(mdv_syncerino *syncerino, mdv_uuid const *trlog)
{
    return MDV_OK;
}
