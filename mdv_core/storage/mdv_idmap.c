#include "mdv_idmap.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <stdatomic.h>


struct mdv_idmap
{
    mdv_storage            *storage;
    size_t                  size;
    atomic_uint_fast64_t    ids[1];
};


mdv_idmap * mdv_idmap_open(mdv_storage *storage, char const *name, size_t size)
{
    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_idmap *idmap = mdv_alloc(offsetof(mdv_idmap, ids) + sizeof(atomic_uint_fast64_t) * size, "idmap");

    if (!idmap)
    {
        MDV_LOGE("No memory for ids map");
        return 0;
    }

    idmap->storage = mdv_storage_retain(storage);
    idmap->size = size;

    mdv_rollbacker_push(rollbacker, mdv_idmap_free, idmap);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(storage);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Try to open table
    mdv_map map = mdv_map_open(&transaction,
                               name,
                               MDV_MAP_CREATE | MDV_MAP_INTEGERKEY);

    if (!mdv_map_ok(map))
    {
        MDV_LOGE("Table '%s' not opened", name);
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &map);

    for(uint32_t i = 0; i < size; ++i)
    {
        uint64_t idx = 0;

        mdv_data k = { sizeof i, (void*)&i };
        mdv_data v = { sizeof idx, &idx };

        if (!mdv_map_get(&map, &transaction, &k, &v))
        {
            if (!mdv_map_put(&map, &transaction, &k, &v))
            {
                MDV_LOGE("OP insertion failed.");
                mdv_rollback(rollbacker);
                return 0;
            }
        }

        // Ids map initialization
        atomic_init(idmap->ids + i, idx);
    }

    if (!mdv_transaction_commit(&transaction))
    {
        MDV_LOGE("OP insertion transaction failed.");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_map_close(&map);

    return idmap;
}


void mdv_idmap_free(mdv_idmap *idmap)
{
    if (idmap)
    {
        mdv_storage_release(idmap->storage);
        mdv_free(idmap, "idmap");
    }
}

