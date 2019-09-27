#include "mdv_cfstorage.h"
#include "mdv_storages.h"
#include "../mdv_config.h"
#include "mdv_idmap.h"
#include <mdv_rollbacker.h>
#include <mdv_limits.h>
#include <mdv_alloc.h>
#include <mdv_string.h>
#include <mdv_filesystem.h>
#include <mdv_tracker.h>
#include <mdv_binn.h>
#include <mdv_types.h>
#include <stdatomic.h>
#include <stddef.h>


/// @cond Doxygen_Suppress

struct mdv_cfstorage
{
    mdv_uuid                uuid;               ///< storage UUID
    uint32_t                nodes_num;          ///< nodes count limit
    mdv_storage            *data;               ///< data storage
    mdv_storage            *tr_log;             ///< transaction log storage
    mdv_idmap              *applied;            ///< transaction logs applied positions
    atomic_uint_fast64_t    top[1];             ///< transaction logs last insertion positions
};


typedef mdv_list_entry(mdv_cfstorage_op) mdv_cfstorage_op_list_entry;


static void mdv_cfstorage_log_top(mdv_cfstorage *cfstorage, size_t size, atomic_uint_fast64_t *arr);

/// @endcond


mdv_cfstorage * mdv_cfstorage_open(mdv_uuid const *uuid, uint32_t nodes_num)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(5);

    mdv_cfstorage *cfstorage = (mdv_cfstorage *)mdv_alloc(offsetof(mdv_cfstorage, top) + sizeof(atomic_uint_fast64_t) * nodes_num, "cfstorage");

    if (!cfstorage)
    {
        MDV_LOGE("No free space of memory for cfstorage");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, cfstorage, "cfstorage");


    cfstorage->uuid = *uuid;
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

    mdv_cfstorage_log_top(cfstorage, nodes_num, cfstorage->top);

    MDV_LOGI("Storage '%s' opened", str_uuid.ptr);

    mdv_rollbacker_free(rollbacker);

    return cfstorage;
}


void mdv_cfstorage_close(mdv_cfstorage *cfstorage)
{
    if(cfstorage)
    {
        mdv_idmap_free(cfstorage->applied);
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


static bool mdv_cfstorage_is_key_deleted(mdv_cfstorage *cfstorage,
                                         mdv_map *map,
                                         mdv_transaction *transaction,
                                         mdv_data const *key)
{
    // TODO: Use bloom filter

    mdv_data k = { key->size, key->ptr };
    mdv_data v = { 0, 0 };

    bool ret = mdv_map_get(map, transaction, &k, &v);

    return ret;
}


uint64_t mdv_cfstorage_new_id(mdv_cfstorage *cfstorage, uint32_t peer_id)
{
    return atomic_fetch_add_explicit(&cfstorage->top[peer_id], 1, memory_order_relaxed) + 1;
}


void mdv_cfstorage_top_id_update(mdv_cfstorage *cfstorage, uint32_t peer_id, uint64_t id)
{
    atomic_store_explicit(&cfstorage->top[peer_id], id, memory_order_relaxed);
}


mdv_uuid const * mdv_cfstorage_uuid(mdv_cfstorage *cfstorage)
{
    return &cfstorage->uuid;
}


bool mdv_cfstorage_log_add(mdv_cfstorage  *cfstorage,
                           uint32_t        peer_id,
                           mdv_list const *ops)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(cfstorage->tr_log);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open transaction log
    mdv_map tr_log = mdv_map_open(&transaction,
                                  MDV_MAP_TRANSACTION_LOG(peer_id),
                                  MDV_MAP_CREATE | MDV_MAP_INTEGERKEY);

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

    mdv_list_foreach(ops, mdv_cfstorage_op, op)
    {
        mdv_rowid rowid =
        {
            .peer = peer_id,
            .id = op->row_id
        };

        mdv_data key = { sizeof rowid, &rowid };

        if (!mdv_cfstorage_is_key_deleted(cfstorage,
                                          &rem_map,
                                          &transaction,
                                          &key))        // Delete op has priority
        {
            mdv_data k = { sizeof rowid.id, &rowid.id };

            if (mdv_map_put_unique(&tr_log, &transaction, &k, &op->op))
            {
                if (peer_id != MDV_LOCAL_ID)
                    mdv_cfstorage_top_id_update(cfstorage, peer_id, op->row_id);
            }
            else
                MDV_LOGW("OP insertion failed.");
        }
    }

    if (!mdv_transaction_commit(&transaction))
    {
        MDV_LOGE("OP insertion transaction failed.");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_map_close(&tr_log);

    mdv_rollbacker_free(rollbacker);

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
        mdv_map map = mdv_map_open(&transaction,
                                   MDV_MAP_TRANSACTION_LOG(i),
                                   MDV_MAP_SILENT | MDV_MAP_INTEGERKEY);

        if (!mdv_map_ok(map))
        {
            atomic_init(arr + i, 0);
            continue;
        }

        mdv_map_foreach_entry entry = {};

        mdv_map_foreach_explicit(transaction, map, entry, MDV_CURSOR_LAST, MDV_CURSOR_NEXT)
        {
            atomic_init(arr + i, *(uint64_t*)entry.key.ptr);
            mdv_map_foreach_break(entry);
        }

        mdv_map_close(&map);
    }

    mdv_transaction_abort(&transaction);
}


static size_t mdv_cfstorage_log_read(mdv_cfstorage *cfstorage,
                                     uint32_t peer_id,
                                     uint64_t pos,
                                     size_t size,
                                     mdv_list *ops)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(cfstorage->tr_log);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open transaction log
    mdv_map tr_log = mdv_map_open(&transaction,
                                  MDV_MAP_TRANSACTION_LOG(peer_id),
                                  MDV_MAP_SILENT | MDV_MAP_INTEGERKEY);

    if (!mdv_map_ok(tr_log))
    {
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &tr_log);

    mdv_map_foreach_entry entry =
    {
        .key =
        {
            .size = sizeof pos,
            .ptr = &pos
        }
    };

    size_t n = 0;

    mdv_map_foreach_explicit(transaction, tr_log, entry, MDV_SET_RANGE, MDV_CURSOR_NEXT)
    {
        mdv_cfstorage_op_list_entry *op = mdv_alloc(sizeof(mdv_cfstorage_op_list_entry) + entry.value.size, "cfstorage_op_list_entry");

        if (!op)
        {
            MDV_LOGE("No memory for TR log entry");
            mdv_map_foreach_break(entry);
        }

        op->data.row_id = *(uint64_t*)entry.key.ptr;
        op->data.op.size = entry.value.size;
        op->data.op.ptr = op + 1;

        memcpy(op->data.op.ptr, entry.value.ptr, entry.value.size);

        mdv_list_emplace_back(ops, (mdv_list_entry_base *)op);

        if(++n >= size)
            mdv_map_foreach_break(entry);
    }

    mdv_map_close(&tr_log);

    mdv_transaction_abort(&transaction);

    mdv_rollbacker_free(rollbacker);

    return n;
}


uint64_t mdv_cfstorage_sync(mdv_cfstorage *cfstorage, uint64_t sync_pos, uint32_t peer_src, uint32_t peer_dst, void *arg, mdv_cfstorage_sync_fn fn)
{
    size_t const batch_size = MDV_CONFIG.datasync.batch_size;

    mdv_list ops = {};

    for(bool ok = true; ok;)
    {
        size_t count = mdv_cfstorage_log_read(cfstorage, peer_src, sync_pos, batch_size, &ops);

        if (!count)     // No data in TR log
            break;

        sync_pos = mdv_list_back(&ops, mdv_cfstorage_op)->row_id + 1;

        ok = fn(arg, peer_src, peer_dst, count, &ops);

        mdv_list_clear(&ops);
    }

    return sync_pos;
}


bool mdv_cfstorage_log_apply(mdv_cfstorage *cfstorage)
{
    // TODO
    return false;
}


uint64_t mdv_cfstorage_log_last_id(mdv_cfstorage *cfstorage, uint32_t peer_id)
{
    if (peer_id >= cfstorage->nodes_num)
    {
        MDV_LOGE("Node identifier is too big: %u", peer_id);
        return 0;
    }
    return atomic_load_explicit(&cfstorage->top[peer_id], memory_order_relaxed);
}
