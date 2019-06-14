#include "mdv_table.h"
#include "mdv_storages.h"
#include "../mdv_config.h"
#include <mdv_binn.h>
#include <mdv_log.h>
#include <mdv_filesystem.h>
#include <mdv_rollbacker.h>


uint32_t mdv_field_size(mdv_field const *fld)
{
    switch(fld->type)
    {
        case MDV_FLD_TYPE_BOOL:     return 1u;
        case MDV_FLD_TYPE_CHAR:     return 1u;
        case MDV_FLD_TYPE_BYTE:     return 1u;
        case MDV_FLD_TYPE_INT8:     return 1u;
        case MDV_FLD_TYPE_UINT8:    return 1u;
        case MDV_FLD_TYPE_INT16:    return 2u;
        case MDV_FLD_TYPE_UINT16:   return 2u;
        case MDV_FLD_TYPE_INT32:    return 4u;
        case MDV_FLD_TYPE_UINT32:   return 4u;
        case MDV_FLD_TYPE_INT64:    return 8u;
        case MDV_FLD_TYPE_UINT64:   return 8u;
    }
}


static binn *binn_table_short_info(mdv_table_base const *table)
{
    binn *obj = binn_object();
    if (!obj)
    {
        MDV_LOGE("binn_table_name failed");
        return obj;
    }

    if (!binn_object_set_str(obj, "N", table->name.ptr))
    {
        MDV_LOGE("binn_table_name failed");
        binn_free(obj);
        return 0;
    }

    return obj;
}


static binn *binn_field(mdv_field const *field)
{
    binn *obj = binn_object();
    if (!obj)
    {
        MDV_LOGE("binn_field failed");
        return obj;
    }

    if (!binn_object_set_uint32(obj, "T", (unsigned int)field->type))
    {
        MDV_LOGE("binn_field failed");
        binn_free(obj);
        return 0;
    }

    return obj;
}


mdv_table_storage mdv_table_create(mdv_storage *metainf_storage, mdv_table_base const *table)
{
    mdv_table_storage table_storage = {};

    mdv_rollbacker(8) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    // Start metainf transaction
    mdv_transaction metainf_transaction = mdv_transaction_start(metainf_storage);
    if (!mdv_transaction_ok(metainf_transaction))
    {
        MDV_LOGE("Metainf storage transaction not started");
        return table_storage;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &metainf_transaction);

    // Open tables list
    mdv_map metainf_map = mdv_map_open(&metainf_transaction, MDV_MAP_TABLES, MDV_MAP_CREATE);
    if (!mdv_map_ok(metainf_map))
    {
        MDV_LOGE("Metainf storage table '%s' not created", MDV_MAP_TABLES);
        mdv_rollback(rollbacker);
        return table_storage;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &metainf_map);

    // Serialize short table information
    {
        binn *obj = binn_table_short_info(table);
        if (!obj)
        {
            MDV_LOGE("table_create failed");
            mdv_rollback(rollbacker);
            return table_storage;
        }

        // Save short table information
        mdv_data const k = { table->name.size, table->name.ptr };
        mdv_data const v = { binn_size(obj),   binn_ptr(obj) };

        if (!mdv_map_put_unique(&metainf_map, &metainf_transaction, &k, &v))
        {
            MDV_LOGE("Table name '%s' isn't unique", table->name.ptr);
            mdv_rollback(rollbacker);
            binn_free(obj);
            return table_storage;
        }

        binn_free(obj);
    }

    // Build table subdirectory
    mdv_stack(char, MDV_PATH_MAX) mpool;
    mdv_stack_clear(mpool);

    static mdv_string const dir_delimeter = mdv_str_static("/");
    mdv_string path = mdv_str_pdup(mpool, MDV_CONFIG.storage.path.ptr);
    path = mdv_str_pcat(mpool, path, dir_delimeter, table->name);

    if (mdv_str_empty(path))
    {
        MDV_LOGE("Path '%s' is too long.", MDV_CONFIG.storage.path.ptr);
        mdv_rollback(rollbacker);
        return table_storage;
    }

    // Create subdirectory for table
    if (!mdv_mkdir(path.ptr))
    {
        MDV_LOGE("Directory for table '%s' wasn't created", table->name.ptr);
        mdv_rollback(rollbacker);
        return table_storage;
    }

    mdv_rollbacker_push(rollbacker, mdv_rmdir, path.ptr);

    // Create table storages
    table_storage.data = mdv_storage_open(path.ptr, MDV_STRG_DATA, MDV_STRG_DATA_MAPS, MDV_STRG_NOSUBDIR);
    table_storage.log.ins = mdv_storage_open(path.ptr, MDV_STRG_INS_LOG, MDV_STRG_INS_LOG_MAPS, MDV_STRG_NOSUBDIR);
    table_storage.log.del = mdv_storage_open(path.ptr, MDV_STRG_DEL_LOG, MDV_STRG_DEL_LOG_MAPS, MDV_STRG_NOSUBDIR);

    mdv_rollbacker_push(rollbacker, mdv_table_close, &table_storage);

    if (!table_storage.data
        || !table_storage.log.ins
        || !table_storage.log.del)
    {
        MDV_LOGE("Srorage for table '%s' wasn't created", table->name.ptr);
        mdv_rollback(rollbacker);
        return table_storage;
    }

    // Start table transaction
    mdv_transaction table_transaction = mdv_transaction_start(table_storage.data);
    if (!mdv_transaction_ok(table_transaction))
    {
        MDV_LOGE("Table storage transaction not started");
        mdv_rollback(rollbacker);
        return table_storage;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &table_transaction);

    // Open table metainf
    mdv_map table_map = mdv_map_open(&table_transaction, MDV_MAP_METAINF, MDV_MAP_CREATE);
    if (!mdv_map_ok(table_map))
    {
        MDV_LOGE("Table metainf storage table '%s' not created", MDV_MAP_METAINF);
        mdv_rollback(rollbacker);
        return table_storage;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &table_map);

    // Serialize table information
    {
        for(uint32_t i = 0; i < table->size; ++i)
        {
            mdv_field const *field = table->fields + i;

            binn *obj = binn_field(field);
            if (!obj)
            {
                MDV_LOGE("table_create failed");
                mdv_rollback(rollbacker);
                return table_storage;
            }

            // Save field information
            mdv_data const k = { field->name.size, field->name.ptr };
            mdv_data const v = { binn_size(obj),   binn_ptr(obj) };

            if (!mdv_map_put_unique(&table_map, &table_transaction, &k, &v))
            {
                MDV_LOGE("Field name '%s' isn't unique", field->name.ptr);
                mdv_rollback(rollbacker);
                binn_free(obj);
                return table_storage;
            }

            binn_free(obj);
        }
    }

    mdv_transaction_commit(&table_transaction);
    mdv_map_close(&table_map);

    mdv_transaction_commit(&metainf_transaction);
    mdv_map_close(&metainf_map);

    return table_storage;
}


void mdv_table_close(mdv_table_storage *storage)
{
    mdv_storage_release(storage->data);
    mdv_storage_release(storage->log.ins);
    mdv_storage_release(storage->log.del);
    storage->data = 0;
    storage->log.ins = 0;
    storage->log.del = 0;
}


bool mdv_table_drop(mdv_storage *metainf_storage, char const *name)
{
    return false;
    // TODO
}
