#include "mdv_cfstorage.h"
#include "mdv_storages.h"
#include "../mdv_config.h"
#include <mdv_rollbacker.h>
#include <mdv_limits.h>
#include <mdv_string.h>
#include <mdv_filesystem.h>


static const uint32_t MDV_CFS_CURSOR = 0;


mdv_cfstorage mdv_cfstorage_create(mdv_uuid const *uuid)
{
    mdv_cfstorage cfstorage = {};

    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_uuid_str(str_uuid);
    mdv_uuid_to_str(uuid, &str_uuid);

    // Build table subdirectory
    mdv_stack(char, MDV_PATH_MAX) mpool;
    mdv_stack_clear(mpool);

    static mdv_string const dir_delimeter = mdv_str_static("/");
    mdv_string path = mdv_str_pdup(mpool, MDV_CONFIG.storage.path.ptr);
    path = mdv_str_pcat(mpool, path, dir_delimeter, str_uuid);

    if (mdv_str_empty(path))
    {
        MDV_LOGE("Path '%s' is too long.", MDV_CONFIG.storage.path.ptr);
        return cfstorage;
    }

    // Create subdirectory for table
    if (!mdv_mkdir(path.ptr))
    {
        MDV_LOGE("Storage directory '%s' wasn't created", str_uuid.ptr);
        return cfstorage;
    }

    mdv_rollbacker_push(rollbacker, mdv_rmdir, path.ptr);

    cfstorage.data = mdv_storage_open(path.ptr, MDV_STRG_DATA, MDV_STRG_DATA_MAPS, MDV_STRG_NOSUBDIR);
    atomic_init(&cfstorage.log.top, 0);
    cfstorage.log.storage = mdv_storage_open(path.ptr, MDV_STRG_TRANSACTION_LOG, MDV_STRG_TRANSACTION_LOG_MAPS, MDV_STRG_NOSUBDIR);

    mdv_rollbacker_push(rollbacker, mdv_cfstorage_close, &cfstorage);

    if (!cfstorage.data
        || !cfstorage.log.storage)
    {
        MDV_LOGE("Storage '%s' wasn't created", str_uuid.ptr);
        mdv_rollback(rollbacker);
        return cfstorage;
    }

    if (!mdv_cfstorage_log_seek(&cfstorage, 0))
    {
        MDV_LOGE("Storage '%s' wasn't initialized", str_uuid.ptr);
        mdv_rollback(rollbacker);
        return cfstorage;
    }

    MDV_LOGI("New storage '%s' created", str_uuid.ptr);

    return cfstorage;
}


mdv_cfstorage mdv_cfstorage_open(mdv_uuid const *uuid)
{
    mdv_cfstorage cfstorage = {};

    mdv_uuid_str(str_uuid);
    mdv_uuid_to_str(uuid, &str_uuid);

    // Build table subdirectory
    mdv_stack(char, MDV_PATH_MAX) mpool;
    mdv_stack_clear(mpool);

    static mdv_string const dir_delimeter = mdv_str_static("/");
    mdv_string path = mdv_str_pdup(mpool, MDV_CONFIG.storage.path.ptr);
    path = mdv_str_pcat(mpool, path, dir_delimeter, str_uuid);

    if (mdv_str_empty(path))
    {
        MDV_LOGE("Path '%s' is too long.", MDV_CONFIG.storage.path.ptr);
        return cfstorage;
    }

    cfstorage.data = mdv_storage_open(path.ptr, MDV_STRG_DATA, MDV_STRG_DATA_MAPS, MDV_STRG_NOSUBDIR);
    atomic_init(&cfstorage.log.top, 0);
    atomic_init(&cfstorage.log.pos, 0);
    cfstorage.log.storage = mdv_storage_open(path.ptr, MDV_STRG_TRANSACTION_LOG, MDV_STRG_TRANSACTION_LOG_MAPS, MDV_STRG_NOSUBDIR);

    if (!cfstorage.data
        || !cfstorage.log.storage)
    {
        MDV_LOGE("Storage '%s' wasn't opened", str_uuid.ptr);
        mdv_cfstorage_close(&cfstorage);
        return cfstorage;
    }

    uint64_t pos = 0;

    if (!mdv_cfstorage_log_tell(&cfstorage, &pos))
    {
        MDV_LOGE("Storage '%s' wasn't opened", str_uuid.ptr);
        mdv_cfstorage_close(&cfstorage);
        return cfstorage;
    }

    atomic_init(&cfstorage.log.pos, pos);

    if (mdv_cfstorage_log_last(&cfstorage, &pos))
        atomic_init(&cfstorage.log.top, pos);

    MDV_LOGI("Storage '%s' opened", str_uuid.ptr);

    return cfstorage;
}


void mdv_cfstorage_close(mdv_cfstorage *cfstorage)
{
    mdv_storage_release(cfstorage->data);
    mdv_storage_release(cfstorage->log.storage);
    cfstorage->data = 0;
    cfstorage->log.storage = 0;
}


bool mdv_cfstorage_drop(mdv_uuid const *uuid)
{
    mdv_uuid_str(str_uuid);
    mdv_uuid_to_str(uuid, &str_uuid);

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
    mdv_map map = mdv_map_open(&transaction, MDV_MAP_REMOVED, 0);
    if (!mdv_map_ok(map))
    {
        MDV_LOGE("CFstorage table '%s' not opened", MDV_MAP_REMOVED);
        return false;
    }

    mdv_data k = { key->size, key->ptr };
    mdv_data v = { 0, 0 };

    bool ret = mdv_map_get(&map, &transaction, &k, &v);

    mdv_map_close(&map);

    return ret;
}


bool mdv_cfstorage_add(mdv_cfstorage *cfstorage, size_t count, mdv_cfstorage_op const *ops)
{
    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(cfstorage->log.storage);
    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open transaction log
    mdv_map tr_log = mdv_map_open(&transaction, MDV_MAP_TRANSACTION_LOG, MDV_MAP_CREATE);
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
            uint64_t key = atomic_fetch_add_explicit(&cfstorage->log.top, 1, memory_order_relaxed) + 1;

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

    return true;
}


bool mdv_cfstorage_rem(mdv_cfstorage *cfstorage, size_t count, mdv_cfstorage_op const *ops)
{
    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(cfstorage->log.storage);
    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open transaction log
    mdv_map map = mdv_map_open(&transaction, MDV_MAP_TRANSACTION_LOG, MDV_MAP_CREATE);
    if (!mdv_map_ok(map))
    {
        MDV_LOGE("CFstorage table '%s' not opened", MDV_STRG_TRANSACTION_LOG);
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &map);

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
            uint64_t key = atomic_fetch_add_explicit(&cfstorage->log.top, 1, memory_order_relaxed) + 1;

            mdv_data k = { sizeof key, &key };
            mdv_data v = { ops[i].op.size, (void*)ops[i].op.ptr };

            if (!mdv_map_put_unique(&map, &transaction, &k, &v))
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

    mdv_map_close(&rem_map);
    mdv_map_close(&map);

    return true;
}


bool mdv_cfstorage_log_seek(mdv_cfstorage *cfstorage, uint64_t pos)
{
    atomic_init(&cfstorage->log.pos, pos);

    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(cfstorage->log.storage);
    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open METAINF table in transaction log
    mdv_map map = mdv_map_open(&transaction, MDV_MAP_METAINF, MDV_MAP_CREATE | MDV_MAP_INTEGERKEY);
    if (!mdv_map_ok(map))
    {
        MDV_LOGE("CFstorage METAINF table '%s' not opened", MDV_STRG_TRANSACTION_LOG);
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &map);

    mdv_data k = { sizeof MDV_CFS_CURSOR, (void*)&MDV_CFS_CURSOR };
    mdv_data v = { sizeof pos, &pos };

    if (!mdv_map_put(&map, &transaction, &k, &v))
    {
        MDV_LOGE("OP insertion failed.");
        mdv_rollback(rollbacker);
        return false;
    }

    if (!mdv_transaction_commit(&transaction))
    {
        MDV_LOGE("OP insertion transaction failed.");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_map_close(&map);

    return true;
}


bool mdv_cfstorage_log_tell(mdv_cfstorage *cfstorage, uint64_t *pos)
{
    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(cfstorage->log.storage);
    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open METAINF table in transaction log
    mdv_map map = mdv_map_open(&transaction, MDV_MAP_METAINF, 0);
    if (!mdv_map_ok(map))
    {
        MDV_LOGE("CFstorage METAINF table in '%s' not opened", MDV_STRG_TRANSACTION_LOG);
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &map);

    mdv_data k = { sizeof MDV_CFS_CURSOR, (void*)&MDV_CFS_CURSOR };
    mdv_data v = {};

    if (!mdv_map_get(&map, &transaction, &k, &v))
    {
        MDV_LOGE("OP insertion failed.");
        mdv_rollback(rollbacker);
        return false;
    }

    *pos = *(uint64_t*)v.ptr;

    mdv_rollback(rollbacker);

    return true;
}


bool mdv_cfstorage_log_last(mdv_cfstorage *cfstorage, uint64_t *pos)
{
    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(cfstorage->log.storage);
    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open transaction log
    mdv_map map = mdv_map_open(&transaction, MDV_MAP_TRANSACTION_LOG, 0);
    if (!mdv_map_ok(map))
    {
        MDV_LOGE("CFstorage dataset '%s' not opened", MDV_MAP_TRANSACTION_LOG);
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &map);

    bool ret = false;

    mdv_map_foreach_explicit(transaction, map, entry, MDV_CURSOR_LAST, MDV_CURSOR_NEXT)
    {
        *pos = *(uint64_t*)entry.key.ptr;
        ret = true;
        mdv_map_foreach_break(entry);
    }

    mdv_rollback(rollbacker);

    return ret;
}


bool mdv_cfstorage_log_apply(mdv_cfstorage *cfstorage, mdv_cfstorage_log_handler handler)
{
    // TODO
    return false;
}
