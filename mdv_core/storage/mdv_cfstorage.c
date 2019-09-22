#include "mdv_cfstorage.h"
#include "mdv_storages.h"
#include "../mdv_config.h"
#include "mdv_idmap.h"
#include <mdv_rollbacker.h>
#include <mdv_limits.h>
#include <mdv_alloc.h>
#include <mdv_string.h>
#include <mdv_filesystem.h>
#include <mdv_binn.h>
#include <stdatomic.h>
#include <stddef.h>


/// @cond Doxygen_Suppress

struct mdv_cfstorage
{
    uint32_t                nodes_num;          ///< cluster size
    mdv_storage            *data;               ///< data storage
    mdv_storage            *tr_log;             ///< transaction log storage
    mdv_idmap              *applied;            ///< transaction logs applied positions
    mdv_idmap              *sync;               ///< transaction logs synchronization positions
    atomic_uint_fast64_t    top[1];             ///< transaction logs last insertion positions
};


static void mdv_cfstorage_log_top(mdv_cfstorage *cfstorage, size_t size, atomic_uint_fast64_t *arr);

/// @endcond


mdv_cfstorage * mdv_cfstorage_open(mdv_uuid const *uuid, uint32_t nodes_num)
{
    mdv_rollbacker(6) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_cfstorage *cfstorage = (mdv_cfstorage *)mdv_alloc(offsetof(mdv_cfstorage, top) + sizeof(atomic_uint_fast64_t) * nodes_num, "cfstorage");

    if (!cfstorage)
    {
        MDV_LOGE("No free space of memory for cfstorage");
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, cfstorage, "cfstorage");


    cfstorage->nodes_num = nodes_num;

    mdv_string const str_uuid = mdv_uuid_to_str(uuid);

    // Build table subdirectory
    mdv_stack(char, MDV_PATH_MAX) mpool;
    mdv_stack_clear(mpool);

    static mdv_string const dir_delimeter = mdv_str_static("/");
    mdv_string path = mdv_str_pdup(mpool, MDV_CONFIG.storage.path.ptr);
    path = mdv_str_pcat(mpool, path, dir_delimeter, str_uuid);

    if (mdv_str_empty(path))
    {
        MDV_LOGE("Path '%s' is too long.", MDV_CONFIG.storage.path.ptr);
        mdv_rollback(rollbacker);
        return 0;
    }

    // Create subdirectory for table
    if (!mdv_mkdir(path.ptr))
    {
        MDV_LOGE("Storage directory '%s' wasn't created", str_uuid.ptr);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_rmdir, path.ptr);


    cfstorage->data = mdv_storage_open(path.ptr,
                                       MDV_STRG_DATA,
                                       MDV_STRG_DATA_MAPS,
                                       MDV_STRG_NOSUBDIR);

    if (!cfstorage->data)
    {
        MDV_LOGE("Storage '%s' wasn't created", str_uuid.ptr);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_storage_release, cfstorage->data);


    cfstorage->tr_log = mdv_storage_open(path.ptr,
                                         MDV_STRG_TRANSACTION_LOG,
                                         MDV_STRG_TRANSACTION_LOG_MAPS(nodes_num),
                                         MDV_STRG_NOSUBDIR);

    if (!cfstorage->tr_log)
    {
        MDV_LOGE("Storage '%s' wasn't created", str_uuid.ptr);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_storage_release, cfstorage->tr_log);


    cfstorage->applied = mdv_idmap_open(cfstorage->tr_log, MDV_MAP_APPLIED_POS, nodes_num);

    if (!cfstorage->applied)
    {
        MDV_LOGE("Map '%s' initialization failed", MDV_MAP_APPLIED_POS);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_idmap_free, cfstorage->applied);


    cfstorage->sync = mdv_idmap_open(cfstorage->tr_log, MDV_MAP_SYNC_POS, nodes_num);

    if (!cfstorage->sync)
    {
        MDV_LOGE("Map '%s' initialization failed", MDV_MAP_SYNC_POS);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_idmap_free, cfstorage->sync);

    mdv_cfstorage_log_top(cfstorage, nodes_num, cfstorage->top);

    MDV_LOGI("New storage '%s' created", str_uuid.ptr);

    return cfstorage;
}


void mdv_cfstorage_close(mdv_cfstorage *cfstorage)
{
    if(cfstorage)
    {
        mdv_idmap_free(cfstorage->applied);
        mdv_idmap_free(cfstorage->sync);
        mdv_storage_release(cfstorage->data);
        mdv_storage_release(cfstorage->tr_log);
        cfstorage->data = 0;
        cfstorage->tr_log = 0;
        mdv_free(cfstorage, "cfstorage");
    }
}


bool mdv_cfstorage_drop(mdv_uuid const *uuid)
{
    mdv_string const str_uuid = mdv_uuid_to_str(uuid);

    // Build table subdirectory
    mdv_stack(char, MDV_PATH_MAX) mpool;
    mdv_stack_clear(mpool);

    static mdv_string const dir_delimeter = mdv_str_static("/");
    mdv_string path = mdv_str_pdup(mpool, MDV_CONFIG.storage.path.ptr);
    path = mdv_str_pcat(mpool, path, dir_delimeter, str_uuid);

    if (mdv_str_empty(path))
    {
        MDV_LOGE("Path '%s' is too long.", MDV_CONFIG.storage.path.ptr);
        return false;
    }

    MDV_LOGI("Storage '%s' dropped", str_uuid.ptr);

    // Remove table storage directories
    if (!mdv_rmdir(path.ptr))
    {
        MDV_LOGE(">>>> Storage directory '%s' wasn't deleted. Delete it manually!!!", str_uuid.ptr);
        return true;
    }

    return true;
}


static bool mdv_cfstorage_is_key_deleted(mdv_cfstorage *cfstorage, mdv_transaction transaction, mdv_data const *key)
{
    // TODO: Use bloom filter

    // Open removed objects table
    mdv_map map = mdv_map_open(&transaction, MDV_MAP_REMOVED, MDV_MAP_SILENT);
    if (!mdv_map_ok(map))
        return false;

    mdv_data k = { key->size, key->ptr };
    mdv_data v = { 0, 0 };

    bool ret = mdv_map_get(&map, &transaction, &k, &v);

    mdv_map_close(&map);

    return ret;
}


bool mdv_cfstorage_add(mdv_cfstorage *cfstorage, uint32_t peer_id, size_t count, mdv_cfstorage_op const *ops)
{
    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(cfstorage->tr_log);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open transaction log
    mdv_map tr_log = mdv_map_open(&transaction, MDV_MAP_TRANSACTION_LOG(peer_id), MDV_MAP_CREATE);

    if (!mdv_map_ok(tr_log))
    {
        MDV_LOGE("CFstorage table '%s' not opened", MDV_STRG_TRANSACTION_LOG);
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &tr_log);

    // Open added objects table
    mdv_map added_map = mdv_map_open(&transaction, MDV_MAP_ADDED, MDV_MAP_CREATE);

    if (!mdv_map_ok(added_map))
    {
        MDV_LOGE("CFstorage table '%s' not opened", MDV_MAP_ADDED);
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &added_map);

    for(size_t i = 0; i < count; ++i)
    {
        // Save new unique object id
        {
            mdv_data k = { ops[i].key.size, ops[i].key.ptr };
            mdv_data v = { 0, 0 };

            if (!mdv_map_put_unique(&added_map, &transaction, &k, &v))
            {
                MDV_LOGE("OP insertion failed.");
                mdv_rollback(rollbacker);
                return false;
            }
        }

        if (!mdv_cfstorage_is_key_deleted(cfstorage, transaction, &ops[i].key))  // Delete op has priority
        {
            uint64_t key = atomic_fetch_add_explicit(&cfstorage->top[peer_id], 1, memory_order_relaxed) + 1;

            mdv_data k = { sizeof key, &key };
            mdv_data v = { ops[i].op.size, (void*)ops[i].op.ptr };

            if (!mdv_map_put_unique(&tr_log, &transaction, &k, &v))
            {
                MDV_LOGE("OP insertion failed.");
                mdv_rollback(rollbacker);
                return false;
            }
        }
    }

    if (!mdv_transaction_commit(&transaction))
    {
        MDV_LOGE("OP insertion transaction failed.");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_map_close(&tr_log);
    mdv_map_close(&added_map);

    return true;
}


bool mdv_cfstorage_rem(mdv_cfstorage *cfstorage, uint32_t peer_id, size_t count, mdv_cfstorage_op const *ops)
{
    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(cfstorage->tr_log);
    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open transaction log
    mdv_map tr_log = mdv_map_open(&transaction, MDV_MAP_TRANSACTION_LOG(peer_id), MDV_MAP_CREATE);
    if (!mdv_map_ok(tr_log))
    {
        MDV_LOGE("CFstorage table '%s' not opened", MDV_STRG_TRANSACTION_LOG);
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &tr_log);

    // Open removed objects table
    mdv_map rem_map = mdv_map_open(&transaction, MDV_MAP_REMOVED, MDV_MAP_CREATE);
    if (!mdv_map_ok(rem_map))
    {
        MDV_LOGE("CFstorage table '%s' not opened", MDV_MAP_REMOVED);
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &rem_map);

    for(size_t i = 0; i < count; ++i)
    {
        mdv_data k = { ops[i].key.size, ops[i].key.ptr };
        mdv_data v = { 0, 0 };

        // if delete request is unique
        if (mdv_map_put_unique(&rem_map, &transaction, &k, &v))
        {
            uint64_t key = atomic_fetch_add_explicit(&cfstorage->top[peer_id], 1, memory_order_relaxed) + 1;

            mdv_data k = { sizeof key, &key };
            mdv_data v = { ops[i].op.size, (void*)ops[i].op.ptr };

            if (!mdv_map_put_unique(&tr_log, &transaction, &k, &v))
            {
                MDV_LOGE("OP insertion failed.");
                mdv_rollback(rollbacker);
                return false;
            }
        }
    }

    if (!mdv_transaction_commit(&transaction))
    {
        MDV_LOGE("OP insertion transaction failed.");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_map_close(&tr_log);
    mdv_map_close(&rem_map);

    return true;
}


void mdv_cfstorage_log_top(mdv_cfstorage *cfstorage, size_t size, atomic_uint_fast64_t *arr)
{
    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(cfstorage->tr_log);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");

        for(size_t i = 0; i < size; ++i)
            atomic_init(arr + i, 0);

        return;
    }

    for(size_t i = 0; i < size; ++i)
    {
        // Open transaction log
        mdv_map map = mdv_map_open(&transaction, MDV_MAP_TRANSACTION_LOG(i), MDV_MAP_SILENT);

        if (!mdv_map_ok(map))
        {
            atomic_init(arr + i, 0);
            continue;
        }

        mdv_map_foreach_explicit(transaction, map, entry, MDV_CURSOR_LAST, MDV_CURSOR_NEXT)
        {
            atomic_init(arr + i, *(uint64_t*)entry.key.ptr);
            mdv_map_foreach_break(entry);
        }

        mdv_map_close(&map);
    }

    mdv_transaction_abort(&transaction);
}


bool mdv_cfstorage_sync(mdv_cfstorage *cfstorage, uint32_t peer_id, void *arg, mdv_cfstorage_sync_fn fn)
{
    if (peer_id >= cfstorage->nodes_num)
    {
        MDV_LOGE("Peer identifier %u is too big", peer_id);
        return false;
    }

//    uint64_t const top = atomic_load(&cfstorage->applied[peer_id].top);
//    uint64_t const pos = atomic_load(&cfstorage->applied[peer_id].pos);
//    uint64_t const new_records_count = top - pos;
//
//    for(uint64_t sync_count = 0; sync_count < new_records_count;)
//    {
//        uint32_t batch_size = MDV_CONFIG.datasync.batch_size < new_records_count
//                                ? MDV_CONFIG.datasync.batch_size
//                                : new_records_count;
//
//        // TODO
//
//        sync_count += batch_size;
//    }

    return false;
}


bool mdv_cfstorage_log_apply(mdv_cfstorage *cfstorage)
{
    // TODO
    return false;
}
