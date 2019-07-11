#include "mdv_cfstorage.h"
#include "mdv_storages.h"
#include "../mdv_config.h"
#include <mdv_rollbacker.h>
#include <mdv_limits.h>
#include <mdv_alloc.h>
#include <mdv_string.h>
#include <mdv_filesystem.h>
#include <mdv_binn.h>
#include <stdatomic.h>
#include <stddef.h>


typedef struct
{
    atomic_uint_fast64_t top;   // last insertion point
    atomic_uint_fast64_t pos;   // applied point
} mdv_cfstorage_applied_pos;


struct mdv_cfstorage
{
    uint32_t                  nodes_num;        // cluster size
    mdv_storage              *data;             // data storage
    mdv_storage              *tr_log;           // transaction log storage
    mdv_cfstorage_applied_pos applied[1];       // transaction logs applied positions
};


static bool mdv_cfstorage_log_seek(mdv_cfstorage *cfstorage, uint32_t first_node, uint32_t last_node, uint64_t pos);
static bool mdv_cfstorage_log_tell(mdv_cfstorage *cfstorage, uint32_t first_node, uint32_t last_node, mdv_cfstorage_applied_pos *pos);
static void mdv_cfstorage_log_last(mdv_cfstorage *cfstorage, uint32_t first_node, uint32_t last_node, mdv_cfstorage_applied_pos *pos);


mdv_cfstorage * mdv_cfstorage_create(mdv_uuid const *uuid, uint32_t nodes_num)
{
    mdv_cfstorage *cfstorage = (mdv_cfstorage *)mdv_alloc(offsetof(mdv_cfstorage, applied) + sizeof(mdv_cfstorage_applied_pos) * nodes_num);

    if (!cfstorage)
    {
        MDV_LOGE("No free space of memory for cfstorage");
        return 0;
    }

    cfstorage->nodes_num = nodes_num;
    cfstorage->data = 0;
    cfstorage->tr_log = 0;

    for(uint32_t i = 0; i < nodes_num; ++i)
    {
        atomic_init(&cfstorage->applied[i].pos, 0);
        atomic_init(&cfstorage->applied[i].top, 0);
    }

    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_rollbacker_push(rollbacker, mdv_cfstorage_close, cfstorage);

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

    cfstorage->data = mdv_storage_open(path.ptr, MDV_STRG_DATA, MDV_STRG_DATA_MAPS, MDV_STRG_NOSUBDIR);
    cfstorage->tr_log = mdv_storage_open(path.ptr, MDV_STRG_TRANSACTION_LOG, MDV_STRG_TRANSACTION_LOG_MAPS(nodes_num), MDV_STRG_NOSUBDIR);

    if (!cfstorage->data
        || !cfstorage->tr_log)
    {
        MDV_LOGE("Storage '%s' wasn't created", str_uuid.ptr);
        mdv_rollback(rollbacker);
        return 0;
    }

    if (!mdv_cfstorage_log_seek(cfstorage, 0, nodes_num, 0))
    {
        MDV_LOGE("Storage '%s' wasn't initialized", str_uuid.ptr);
        mdv_rollback(rollbacker);
        return 0;
    }

    MDV_LOGI("New storage '%s' created", str_uuid.ptr);

    return cfstorage;
}


mdv_cfstorage * mdv_cfstorage_open(mdv_uuid const *uuid, uint32_t nodes_num)
{
    mdv_cfstorage *cfstorage = (mdv_cfstorage *)mdv_alloc(offsetof(mdv_cfstorage, applied) + sizeof(mdv_cfstorage_applied_pos) * nodes_num);

    if (!cfstorage)
    {
        MDV_LOGE("No free space of memory for cfstorage");
        return 0;
    }

    cfstorage->nodes_num = nodes_num;
    cfstorage->data = 0;
    cfstorage->tr_log = 0;

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
        mdv_cfstorage_close(cfstorage);
        return 0;
    }

    cfstorage->data = mdv_storage_open(path.ptr, MDV_STRG_DATA, MDV_STRG_DATA_MAPS, MDV_STRG_NOSUBDIR);
    cfstorage->tr_log = mdv_storage_open(path.ptr, MDV_STRG_TRANSACTION_LOG, MDV_STRG_TRANSACTION_LOG_MAPS(nodes_num), MDV_STRG_NOSUBDIR);

    if (!cfstorage->data
        || !cfstorage->tr_log)
    {
        MDV_LOGE("Storage '%s' wasn't opened", str_uuid.ptr);
        mdv_cfstorage_close(cfstorage);
        return 0;
    }

    if (!mdv_cfstorage_log_tell(cfstorage, 0, nodes_num, cfstorage->applied))
    {
        MDV_LOGE("Storage '%s' wasn't opened", str_uuid.ptr);
        mdv_cfstorage_close(cfstorage);
        return 0;
    }

    mdv_cfstorage_log_last(cfstorage, 0, nodes_num, cfstorage->applied);

    MDV_LOGI("Storage '%s' opened", str_uuid.ptr);

    return cfstorage;
}


void mdv_cfstorage_close(mdv_cfstorage *cfstorage)
{
    if(cfstorage)
    {
        mdv_storage_release(cfstorage->data);
        mdv_storage_release(cfstorage->tr_log);
        cfstorage->data = 0;
        cfstorage->tr_log = 0;
        mdv_free(cfstorage);
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


bool mdv_cfstorage_add(mdv_cfstorage *cfstorage, uint32_t node_id, size_t count, mdv_cfstorage_op const *ops)
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
    mdv_map tr_log = mdv_map_open(&transaction, MDV_MAP_TRANSACTION_LOG(node_id), MDV_MAP_CREATE);
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
            uint64_t key = atomic_fetch_add_explicit(&cfstorage->applied[node_id].top, 1, memory_order_relaxed) + 1;

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


bool mdv_cfstorage_rem(mdv_cfstorage *cfstorage, uint32_t node_id, size_t count, mdv_cfstorage_op const *ops)
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
    mdv_map map = mdv_map_open(&transaction, MDV_MAP_TRANSACTION_LOG(node_id), MDV_MAP_CREATE);
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
            uint64_t key = atomic_fetch_add_explicit(&cfstorage->applied[node_id].top, 1, memory_order_relaxed) + 1;

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


bool mdv_cfstorage_log_seek(mdv_cfstorage *cfstorage, uint32_t first_node, uint32_t last_node, uint64_t pos)
{
    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(cfstorage->tr_log);
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

    for(uint32_t i = first_node; i < last_node; ++i)
    {
        atomic_init(&cfstorage->applied[i].pos, pos);

        mdv_data k = { sizeof i, (void*)&i };
        mdv_data v = { sizeof pos, &pos };

        if (!mdv_map_put(&map, &transaction, &k, &v))
        {
            MDV_LOGE("OP insertion failed.");
            mdv_rollback(rollbacker);
            return false;
        }
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


bool mdv_cfstorage_log_tell(mdv_cfstorage *cfstorage, uint32_t first_node, uint32_t last_node, mdv_cfstorage_applied_pos *positions)
{
    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(cfstorage->tr_log);
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

    for(uint32_t i = first_node; i < last_node; ++i)
    {
        mdv_data k = { sizeof i, (void*)&i };
        mdv_data v = {};

        if (!mdv_map_get(&map, &transaction, &k, &v))
        {
            MDV_LOGE("OP insertion failed.");
            mdv_rollback(rollbacker);
            return false;
        }

        atomic_init(&positions[i].pos, *(uint64_t*)v.ptr);
    }

    mdv_rollback(rollbacker);

    return true;
}


void mdv_cfstorage_log_last(mdv_cfstorage *cfstorage, uint32_t first_node, uint32_t last_node, mdv_cfstorage_applied_pos *positions)
{
    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(cfstorage->tr_log);
    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        return;
    }

    for(uint32_t i = first_node; i < last_node; ++i)
    {
        // Open transaction log
        mdv_map map = mdv_map_open(&transaction, MDV_MAP_TRANSACTION_LOG(i), MDV_MAP_SILENT);
        if (!mdv_map_ok(map))
            continue;

        mdv_map_foreach_explicit(transaction, map, entry, MDV_CURSOR_LAST, MDV_CURSOR_NEXT)
        {
            atomic_init(&positions[i].top, *(uint64_t*)entry.key.ptr);
            mdv_map_foreach_break(entry);
        }

        mdv_map_close(&map);
    }

    mdv_transaction_abort(&transaction);
}


bool mdv_cfstorage_log_apply(mdv_cfstorage *cfstorage, mdv_cfstorage_log_handler handler)
{
    // TODO
    return false;
}
