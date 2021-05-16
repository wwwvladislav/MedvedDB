#include "mdv_trlog.h"
#include <mdv_lmdb.h>
#include <mdv_names.h>
#include "../mdv_config.h"
#include "../event/mdv_evt_types.h"
#include "../event/mdv_evt_trlog.h"
#include <mdv_rollbacker.h>
#include <mdv_alloc.h>
#include <mdv_string.h>
#include <mdv_limits.h>
#include <mdv_log.h>
#include <mdv_filesystem.h>
#include <stdatomic.h>
#include <assert.h>


static const uint32_t MDV_TRLOG_APPLIED_POS_KEY = 0;


/// Transaction logs storage
struct mdv_trlog
{
    mdv_uuid                uuid;               ///< storage UUID
    uint32_t                id;                 ///< storage local unique identifier
    mdv_lmdb               *storage;            ///< transaction log storage
    mdv_ebus               *ebus;               ///< Events bus
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
        MDV_LOGE("TR log transaction not started");
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
            break;

        mdv_map_foreach_entry entry = {};

        mdv_map_foreach_explicit(transaction, map, entry, MDV_CURSOR_LAST, MDV_CURSOR_NEXT)
        {
            atomic_init(&trlog->top, *(uint64_t*)entry.key.ptr + 1);
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
            break;

        mdv_data const key = { sizeof MDV_TRLOG_APPLIED_POS_KEY, (void*)&MDV_TRLOG_APPLIED_POS_KEY };
        mdv_data value = {};

        if (mdv_map_get(&map, &transaction, &key, &value))
            atomic_init(&trlog->applied, *(uint64_t*)value.ptr);

        mdv_map_close(&map);
    } while(0);

    mdv_transaction_abort(&transaction);
}


static void mdv_trlog_changed_notify(mdv_trlog *trlog)
{
    mdv_evt_trlog_changed *evt = mdv_evt_trlog_changed_create(&trlog->uuid);

    if (evt)
    {
        mdv_ebus_publish(trlog->ebus, &evt->base, MDV_EVT_UNIQUE);
        mdv_evt_trlog_changed_release(evt);
    }
}


static const mdv_event_handler_type mdv_trlog_handlers[] =
{
};


mdv_trlog * mdv_trlog_open(mdv_ebus *ebus, char const *dir, mdv_uuid const *uuid, uint32_t id)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_trlog *trlog = mdv_alloc(sizeof(mdv_trlog));

    if (!trlog)
    {
        MDV_LOGE("No free space of memory for trlog");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, trlog);

    trlog->uuid = *uuid;
    trlog->id = id;

    char storage_name[64];

    trlog->storage = mdv_storage_open(dir,
                                      MDV_STRG_UUID(uuid, storage_name, sizeof storage_name),
                                      MDV_STRG_TRLOG_MAPS,
                                      MDV_STRG_NOSUBDIR,
                                      MDV_CONFIG.storage.max_size);

    if (!trlog->storage)
    {
        MDV_LOGE("Storage '%s' wasn't created", storage_name);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_trlog_init(trlog);

    mdv_rollbacker_push(rollbacker, mdv_storage_release, trlog->storage);

    trlog->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, trlog->ebus);

    if (mdv_ebus_subscribe_all(trlog->ebus,
                               trlog,
                               mdv_trlog_handlers,
                               sizeof mdv_trlog_handlers / sizeof *mdv_trlog_handlers) != MDV_OK)
    {
        MDV_LOGE("Ebus subscription failed");
        mdv_rollback(rollbacker);
        return 0;
    }

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
    if (!trlog)
        return 0;

    uint32_t rc = mdv_storage_release(trlog->storage);

    if (!rc)
    {
        mdv_ebus_unsubscribe_all(trlog->ebus,
                                 trlog,
                                 mdv_trlog_handlers,
                                 sizeof mdv_trlog_handlers / sizeof *mdv_trlog_handlers);

        mdv_ebus_release(trlog->ebus);
        mdv_free(trlog);
    }

    return rc;
}


uint32_t mdv_trlog_id(mdv_trlog *trlog)
{
    return trlog->id;
}


mdv_uuid const * mdv_trlog_uuid(mdv_trlog *trlog)
{
    return &trlog->uuid;
}


uint64_t mdv_trlog_top(mdv_trlog *trlog)
{
    return atomic_load(&trlog->top);
}


static uint64_t mdv_trlog_new_id(mdv_trlog *trlog)
{
    return atomic_fetch_add_explicit(&trlog->top, 1, memory_order_relaxed);
}


static void mdv_trlog_id_maximize(mdv_trlog *trlog, uint64_t id)
{
    ++id;

    for(uint64_t top_id = atomic_load(&trlog->top); id > top_id;)
    {
        if(atomic_compare_exchange_weak(&trlog->top, &top_id, id))
            break;
    }
}


static void mdv_trlog_applied_pos_set(mdv_trlog *trlog, uint64_t applied_pos)
{
    atomic_store_explicit(&trlog->applied, applied_pos, memory_order_relaxed);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(trlog->storage);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("TR log transaction not started");
        return;
    }

    // Open transaction log
    mdv_map map = mdv_map_open(&transaction,
                                MDV_MAP_APPLIED,
                                MDV_MAP_CREATE | MDV_MAP_INTEGERKEY);

    if (!mdv_map_ok(map))
    {
        mdv_transaction_abort(&transaction);
        return;
    }

    // Save transaction log application position
    mdv_data const key = { sizeof MDV_TRLOG_APPLIED_POS_KEY, (void*)&MDV_TRLOG_APPLIED_POS_KEY };
    mdv_data value = { sizeof applied_pos, &applied_pos };

    if (mdv_map_put(&map, &transaction, &key, &value))
    {
        if (!mdv_transaction_commit(&transaction))
            MDV_LOGE("Transaction failed.");
        mdv_map_close(&map);
    }
    else
    {
        MDV_LOGE("TR log applied posiotion wasn't saved");
        mdv_map_close(&map);
        mdv_transaction_abort(&transaction);
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

    bool changed = false;

    mdv_list_foreach(ops, mdv_trlog_data, op)
    {
        mdv_data k = { sizeof op->id, &op->id };
        mdv_data v = { op->op.size, &op->op };

        if (mdv_map_put_unique(&tr_log, &transaction, &k, &v))
        {
            changed = true;
            mdv_trlog_id_maximize(trlog, op->id);
        }
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

    if (changed)
        mdv_trlog_changed_notify(trlog);

    return true;
}


bool mdv_trlog_add_op(mdv_trlog *trlog,
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

    mdv_trlog_changed_notify(trlog);

    return true;
}


static size_t mdv_trlog_read(mdv_trlog                    *trlog,
                             uint64_t                      pos,
                             size_t                        size,
                             mdv_list/*<mdv_trlog_data>*/ *ops)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(trlog->storage);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("TR log transaction failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open transaction log
    mdv_map tr_log = mdv_map_open(&transaction,
                                  MDV_MAP_TRLOG,
                                  MDV_MAP_SILENT | MDV_MAP_INTEGERKEY);

    if (!mdv_map_ok(tr_log))
    {
        MDV_LOGE("Transaction log map '%s' not opened", MDV_MAP_TRLOG);
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

    mdv_map_foreach_explicit(transaction, tr_log, entry, MDV_CURSOR_SET_RANGE, MDV_CURSOR_NEXT)
    {
        uint64_t const id = *(uint64_t*)entry.key.ptr;

        mdv_trlog_entry *op = mdv_alloc(sizeof(mdv_trlog_entry) + entry.value.size);

        if (!op)
        {
            MDV_LOGE("No memory for TR log entry");
            mdv_map_foreach_break(entry);
        }

        mdv_trlog_op const *trlog_op = entry.value.ptr;

        op->data.id = id;

        assert(trlog_op->size == entry.value.size);

        memcpy(&op->data.op, trlog_op, trlog_op->size);

        mdv_list_emplace_back(ops, (mdv_list_entry_base *)op);

        if(++n >= size)
            mdv_map_foreach_break(entry);
    }

    mdv_map_close(&tr_log);

    mdv_transaction_abort(&transaction);

    mdv_rollbacker_free(rollbacker);

    return n;
}


size_t mdv_trlog_range_read(mdv_trlog                    *trlog,
                            uint64_t                      from,
                            uint64_t                      to,
                            mdv_list/*<mdv_trlog_data>*/ *ops)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(trlog->storage);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("TR log transaction failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open transaction log
    mdv_map tr_log = mdv_map_open(&transaction,
                                  MDV_MAP_TRLOG,
                                  MDV_MAP_SILENT | MDV_MAP_INTEGERKEY);

    if (!mdv_map_ok(tr_log))
    {
        MDV_LOGE("Transaction log map '%s' not opened", MDV_MAP_TRLOG);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &tr_log);

    mdv_map_foreach_entry entry =
    {
        .key =
        {
            .size = sizeof from,
            .ptr = &from
        }
    };

    size_t n = 0;

    mdv_map_foreach_explicit(transaction, tr_log, entry, MDV_CURSOR_SET_RANGE, MDV_CURSOR_NEXT)
    {
        uint64_t const id = *(uint64_t*)entry.key.ptr;

        if(id >= to)
            mdv_map_foreach_break(entry);

        mdv_trlog_entry *op = mdv_alloc(sizeof(mdv_trlog_entry) + entry.value.size);

        if (!op)
        {
            MDV_LOGE("No memory for TR log entry");
            mdv_map_foreach_break(entry);
        }

        mdv_trlog_op const *trlog_op = entry.value.ptr;

        op->data.id = id;

        assert(trlog_op->size == entry.value.size);

        memcpy(&op->data.op, trlog_op, trlog_op->size);

        mdv_list_emplace_back(ops, (mdv_list_entry_base *)op);

        ++n;
    }

    mdv_map_close(&tr_log);

    mdv_transaction_abort(&transaction);

    mdv_rollbacker_free(rollbacker);

    return n;
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
    uint64_t const applied_pos = atomic_load_explicit(&trlog->applied, memory_order_relaxed);
    uint64_t const top         = atomic_load_explicit(&trlog->top, memory_order_relaxed);

    if (applied_pos >= top)
        return 0;

    mdv_list/*<mdv_trlog_data>*/ ops = {};

    size_t const count = mdv_trlog_read(trlog, applied_pos, batch_size, &ops);

    if (!count)     // No data in TR log
        return 0;

    uint64_t new_applied_pos = applied_pos;

    size_t n = 0;

    mdv_list_foreach(&ops, mdv_trlog_data, op)
    {
        if (!fn(arg, &op->op))
        {
            MDV_LOGE("TR Log operation not applied");
            break;
        }

        new_applied_pos = op->id + 1;
        ++n;
    }

    mdv_list_clear(&ops);

    if (n)
        mdv_trlog_applied_pos_set(trlog, new_applied_pos);

    return n;
}


uint32_t mdv_trlog_foreach(mdv_trlog   *trlog,
                           uint64_t     id,
                           uint32_t     batch_size,
                           void        *arg,
                           mdv_trlog_fn fn)
{
    mdv_list/*<mdv_trlog_data>*/ ops = {};

    size_t const count = mdv_trlog_read(trlog, id, batch_size, &ops);

    if (!count)     // No data in TR log
        return 0;

    size_t n = 0;

    mdv_list_foreach(&ops, mdv_trlog_data, op)
    {
        if (!fn(arg, op))
        {
            MDV_LOGE("TR Log items iteration failed");
            break;
        }

        ++n;
    }

    mdv_list_clear(&ops);

    return n;
}
