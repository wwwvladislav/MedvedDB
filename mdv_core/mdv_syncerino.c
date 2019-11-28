#include "mdv_syncerino.h"
#include "mdv_syncerlog.h"
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
    mdv_jobber     *jobber;         ///< Jobs scheduler
    mdv_uuid        uuid;           ///< Current node UUID
    mdv_uuid        peer;           ///< Global unique identifier for peer
    mdv_mutex       mutex;          ///< Mutex for TR log synchronizers guard
    mdv_hashmap    *trlogs;         ///< Current synchronizing TR logs (hashmap<mdv_syncerlog_ref>)
};


typedef struct
{
    mdv_uuid        uuid;           ///< TR log UUID
    mdv_syncerlog  *syncerlog;      ///< TR log changes counter
} mdv_syncerlog_ref;


mdv_syncerino * mdv_syncerino_create(mdv_uuid const *uuid, mdv_uuid const *peer, mdv_ebus *ebus, mdv_jobber *jobber)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(5);

    mdv_syncerino *syncerino = mdv_alloc(sizeof(mdv_syncerino), "syncerino");

    if (!syncerino)
    {
        MDV_LOGE("No memory for new syncerino");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, syncerino, "syncerino");

    atomic_init(&syncerino->rc, 1);
    syncerino->uuid = *uuid;
    syncerino->peer = *peer;

    syncerino->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, syncerino->ebus);

    syncerino->jobber = mdv_jobber_retain(jobber);

    mdv_rollbacker_push(rollbacker, mdv_jobber_release, syncerino->jobber);

    if (mdv_mutex_create(&syncerino->mutex) != MDV_OK)
    {
        MDV_LOGE("Mutex for TR log synchronizers not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &syncerino->mutex);

    syncerino->trlogs = mdv_hashmap_create(mdv_syncerlog_ref,
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
    mdv_jobber_release(syncerino->jobber);
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
{
    if (mdv_mutex_lock(&syncerino->mutex) == MDV_OK)
    {
        mdv_hashmap_foreach(syncerino->trlogs, mdv_syncerlog_ref, ref)
            mdv_syncerlog_release(ref->syncerlog);
        mdv_hashmap_clear(syncerino->trlogs);
        mdv_mutex_unlock(&syncerino->mutex);
    }
}


static mdv_syncerlog * mdv_syncerino_syncerlog(mdv_syncerino *syncerino, mdv_uuid const *trlog)
{
    if (mdv_uuid_cmp(trlog, &syncerino->peer) == 0)
        return 0;

    mdv_syncerlog_ref *ref = 0;

    if (mdv_mutex_lock(&syncerino->mutex) == MDV_OK)
    {
        ref = mdv_hashmap_find(syncerino->trlogs, trlog);

        if (!ref)
        {
            mdv_syncerlog_ref new_ref =
            {
                .uuid = *trlog,
                .syncerlog = mdv_syncerlog_create(&syncerino->uuid, &syncerino->peer, trlog, syncerino->ebus, syncerino->jobber)
            };

            if (new_ref.syncerlog)
            {
                ref = mdv_hashmap_insert(syncerino->trlogs, &new_ref, sizeof new_ref);

                if (!ref)
                {
                    mdv_syncerlog_release(new_ref.syncerlog);
                    MDV_LOGE("Transaction log synchronizer creation failed");
                }
            }
            else
                MDV_LOGE("Transaction log synchronizer creation failed");
        }

        mdv_mutex_unlock(&syncerino->mutex);
    }

    return ref ? mdv_syncerlog_retain(ref->syncerlog) : 0;
}


mdv_errno mdv_syncerino_start(mdv_syncerino *syncerino, mdv_uuid const *trlog)
{
    if (mdv_uuid_cmp(trlog, &syncerino->peer) == 0)
        return MDV_OK;

    mdv_syncerlog *syncerlog = mdv_syncerino_syncerlog(syncerino, trlog);

    if (syncerlog)
    {
        mdv_syncerlog_release(syncerlog);
        return MDV_OK;
    }

    return MDV_FAILED;
}
