#include "mdv_trlog.h"
#include "mdv_storage.h"
#include "mdv_storages.h"
#include "mdv_idmap.h"
#include <mdv_rollbacker.h>
#include <mdv_alloc.h>
#include <mdv_string.h>
#include <mdv_limits.h>
#include <mdv_log.h>
#include <mdv_filesystem.h>
#include <stdatomic.h>


/// Transaction logs storage
struct mdv_trlog
{
    mdv_uuid                uuid;               ///< storage UUID
    mdv_storage            *storage;            ///< transaction log storage
    atomic_uint_fast64_t    top;                ///< transaction log last insertion position
    atomic_uint_fast64_t    applied;            ///< transaction log application position
};


static void mdv_trlog_init(mdv_trlog *trlog)
{
    atomic_init(&trlog->top, 0);
    atomic_init(&trlog->applied, 0);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(trlog->storage);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        return;
    }

    // Get transaction log last insertion position
    do
    {
        // Open transaction log
        mdv_map map = mdv_map_open(&transaction,
                                    MDV_MAP_TRLOG,
                                    MDV_MAP_SILENT | MDV_MAP_INTEGERKEY);

        if (!mdv_map_ok(map))
        {
            mdv_transaction_abort(&transaction);
            break;
        }

        mdv_map_foreach_entry entry = {};

        mdv_map_foreach_explicit(transaction, map, entry, MDV_CURSOR_LAST, MDV_CURSOR_NEXT)
        {
            atomic_init(&trlog->top, *(uint64_t*)entry.key.ptr);
            mdv_map_foreach_break(entry);
        }

        mdv_map_close(&map);
    } while(0);

    // Get transaction log application position
    do
    {
        // Open transaction log
        mdv_map map = mdv_map_open(&transaction,
                                    MDV_MAP_APPLIED,
                                    MDV_MAP_SILENT | MDV_MAP_INTEGERKEY);

        if (!mdv_map_ok(map))
        {
            mdv_transaction_abort(&transaction);
            break;
        }

        mdv_map_foreach(transaction, map, entry)
        {
            atomic_init(&trlog->applied, *(uint64_t*)entry.value.ptr);
            mdv_map_foreach_break(entry);
        }

        mdv_map_close(&map);
    } while(0);

    mdv_transaction_abort(&transaction);
}


mdv_trlog * mdv_trlog_open(mdv_uuid const *uuid, char const *root_dir)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_trlog *trlog = mdv_alloc(sizeof(mdv_trlog), "trlog");

    if (!trlog)
    {
        MDV_LOGE("No free space of memory for trlog");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, trlog, "trlog");

    trlog->uuid = *uuid;

    atomic_init(&trlog->top, 0);

    // Build trlog subdirectory
    mdv_stack(char, MDV_PATH_MAX) mpool;
    mdv_stack_clear(mpool);

    static mdv_string const trlogs_dir = mdv_str_static("/trlog");
    mdv_string path = mdv_str_pdup(mpool, root_dir);
    path = mdv_str_pcat(mpool, path, trlogs_dir);

    if (mdv_str_empty(path))
    {
        MDV_LOGE("Path '%s' is too long.", root_dir);
        mdv_rollback(rollbacker);
        return 0;
    }

    // Create subdirectory for table
    if (!mdv_mkdir(path.ptr))
    {
        MDV_LOGE("TR logs directory wasn't created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_rmdir, path.ptr);


    trlog->storage = mdv_storage_open(path.ptr,
                                      MDV_STRG_TRLOG(uuid),
                                      MDV_STRG_TRLOG_MAPS,
                                      MDV_STRG_NOSUBDIR);

    if (!trlog->storage)
    {
        MDV_LOGE("Storage '%s' wasn't created", MDV_STRG_TRLOG(uuid));
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_trlog_init(trlog);

    mdv_rollbacker_push(rollbacker, mdv_storage_release, trlog->storage);

    mdv_rollbacker_free(rollbacker);

    return trlog;
}


mdv_trlog * mdv_trlog_retain(mdv_trlog *trlog)
{
    if (trlog)
        mdv_storage_retain(trlog->storage);
    return trlog;
}


uint32_t mdv_trlog_release(mdv_trlog *trlog)
{
    uint32_t rc = mdv_storage_release(trlog->storage);

    if (!rc)
        mdv_free(trlog, "trlog");

    return rc;
}


static uint64_t mdv_trlog_new_id(mdv_trlog *trlog)
{
    return atomic_fetch_add_explicit(&trlog->top, 1, memory_order_relaxed) + 1;
}


void mdv_trlog_id_maximize(mdv_trlog *trlog, uint64_t id)
{
    for(uint64_t top_id = atomic_load(&trlog->top); id > top_id;)
    {
        if(atomic_compare_exchange_weak(&trlog->top, &top_id, id))
            break;
    }
}


bool mdv_trlog_add(mdv_trlog *trlog,
                   mdv_list/*<mdv_trlog_data>*/ const *ops)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(trlog->storage);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("TR log transaction failed");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open transaction log
    mdv_map tr_log = mdv_map_open(&transaction,
                                  MDV_MAP_TRLOG,
                                  MDV_MAP_CREATE | MDV_MAP_INTEGERKEY);

    if (!mdv_map_ok(tr_log))
    {
        MDV_LOGE("Transaction log map '%s' not opened", MDV_MAP_TRLOG);
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &tr_log);

    mdv_list_foreach(ops, mdv_trlog_data, op)
    {
        mdv_data k = { sizeof op->id, &op->id };
        mdv_data v = { op->op.size, &op->op };

        if (mdv_map_put_unique(&tr_log, &transaction, &k, &v))
            mdv_trlog_id_maximize(trlog, op->id);
        else
            MDV_LOGW("OP insertion failed.");
    }

    if (!mdv_transaction_commit(&transaction))
    {
        MDV_LOGE("TR log transaction failed");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_map_close(&tr_log);

    mdv_rollbacker_free(rollbacker);

    return true;
}


bool mdv_trlog_new_op(mdv_trlog *trlog,
                      mdv_trlog_op const *op)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(trlog->storage);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("TR log transaction failed");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open transaction log
    mdv_map tr_log = mdv_map_open(&transaction,
                                  MDV_MAP_TRLOG,
                                  MDV_MAP_CREATE | MDV_MAP_INTEGERKEY);

    if (!mdv_map_ok(tr_log))
    {
        MDV_LOGE("Transaction log map '%s' not opened", MDV_MAP_TRLOG);
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &tr_log);

    uint64_t id = mdv_trlog_new_id(trlog);

    mdv_data k = { sizeof id, &id };
    mdv_data v = { op->size, (void*)op };

    if (!mdv_map_put_unique(&tr_log, &transaction, &k, &v))
    {
        MDV_LOGW("OP insertion failed.");
        mdv_rollback(rollbacker);
        return false;
    }

    if (!mdv_transaction_commit(&transaction))
    {
        MDV_LOGE("TR log transaction failed");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_map_close(&tr_log);

    mdv_rollbacker_free(rollbacker);

    return true;
}


bool mdv_trlog_changed(mdv_trlog *trlog)
{
    return atomic_load_explicit(&trlog->top, memory_order_relaxed)
            > atomic_load_explicit(&trlog->applied, memory_order_relaxed);
}


uint32_t mdv_trlog_apply(mdv_trlog         *trlog,
                         uint32_t           batch_size,
                         void              *arg,
                         mdv_trlog_apply_fn fn)
{
    // TODO:
    return 0;
}
