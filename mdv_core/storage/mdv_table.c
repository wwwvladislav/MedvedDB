#include "mdv_table.h"
#include "mdv_storages.h"
#include "../mdv_config.h"
#include <mdv_binn.h>
#include <mdv_log.h>
#include <mdv_filesystem.h>
#include <mdv_rollbacker.h>
#include <mdv_alloc.h>


uint32_t mdv_field_type_size(mdv_field_type t)
{
    switch(t)
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
        case MDV_FLD_TYPE_FLOAT:    return 4u;
        case MDV_FLD_TYPE_DOUBLE:   return 8u;
    }
    MDV_LOGE("Unknown type: %u", t);
    return 0;
}


static mdv_table_base * mdv_table_clone(mdv_table_base const *table)
{
    // Calculate size
    size_t size = (char const *)(table->fields + table->size) - (char const *)table;
    size_t strings_size = table->name.size;
    for(uint32_t i = 0; i < table->size; ++i)
        strings_size += table->fields[i].name.size;

    mdv_table_base *clone = mdv_alloc(size + strings_size);
    if (!clone)
    {
        MDV_LOGE("mdv_table_clone failed");
        return 0;
    }

    char *buff = (char*)clone + size;

    memcpy(clone, table, size);

    clone->name.ptr = buff;
    buff += clone->name.size;
    memcpy(clone->name.ptr, table->name.ptr, clone->name.size);

    for(uint32_t i = 0; i < table->size; ++i)
    {
        clone->fields[i].name.ptr = buff;
        buff += table->fields[i].name.size;
        memcpy(clone->fields[i].name.ptr, table->fields[i].name.ptr, table->fields[i].name.size);
    }

    if (buff - (char*)clone != size + strings_size)
        MDV_LOGE("memory is corrupted: %p, %zu != %zu", clone, buff - (char*)clone, size + strings_size);

    return clone;
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

    if (!binn_object_set_uint32(obj, "T", (unsigned int)field->type)
        || !binn_object_set_uint32(obj, "L", (unsigned int)field->limit)
        || !binn_object_set_str(obj, "N", field->name.ptr))
    {
        MDV_LOGE("binn_field failed");
        binn_free(obj);
        return 0;
    }

    return obj;
}


static binn* binn_table(mdv_table_base const *table)
{
    // Calculate size
    uint32_t size = (char const *)(table->fields + table->size) - (char const *)table;
    size += table->name.size;
    for(uint32_t i = 0; i < table->size; ++i)
        size += table->fields[i].name.size;

    binn *obj = binn_object();
    if (!obj)
    {
        MDV_LOGE("binn_table failed");
        return obj;
    }

    if (!binn_object_set_blob(obj, "U", (void *)&table->uuid, sizeof(table->uuid))
        || !binn_object_set_str(obj, "N", table->name.ptr)
        || !binn_object_set_uint32(obj, "S", table->size)
        || !binn_object_set_uint32(obj, "B", size))
    {
        MDV_LOGE("binn_table failed");
        binn_free(obj);
        return 0;
    }

    binn *fields = binn_list();
    if (!fields)
    {
        MDV_LOGE("binn_table failed");
        binn_free(obj);
        return 0;
    }

    for(uint32_t i = 0; i < table->size; ++i)
    {
        binn *field = binn_field(table->fields + i);
        if(!field
           || !binn_list_add_object(fields, field))
        {
            binn_free(fields);
            binn_free(obj);
            return 0;
        }
        binn_free(field);
    }

    if (!binn_object_set_list(obj, "F", fields))
    {
        binn_free(fields);
        binn_free(obj);
        return 0;
    }

    binn_free(fields);

    return obj;
}


static mdv_table_base * unbinn_table(binn *obj)
{
    uint32_t size = 0;
    if (!binn_object_get_uint32(obj, "B", &size))
    {
        MDV_LOGE("unbinn_table failed");
        return 0;
    }

    mdv_table_base *table = (mdv_table_base *)mdv_alloc(size);
    if (!table)
    {
        MDV_LOGE("unbinn_table failed");
        return 0;
    }

    char *name = 0;
    mdv_uuid *uuid = 0;

    if (!binn_object_get_blob(obj, "U", (void *)&uuid, 0)
        || !binn_object_get_str(obj, "N", &name)
        || !binn_object_get_uint32(obj, "S", &table->size))
    {
        MDV_LOGE("unbinn_table failed");
        mdv_free(table);
        return 0;
    }

    table->uuid = *uuid;

    char *buff = (char *)(table->fields + table->size);

    table->name.size = strlen(name) + 1;
    table->name.ptr = buff;
    buff += table->name.size;
    memcpy(table->name.ptr, name, table->name.size);

    binn *fields = 0;
    if (!binn_object_get_list(obj, "F", (void**)&fields))
    {
        MDV_LOGE("unbinn_table failed");
        mdv_free(table);
        return 0;
    }

    binn_iter iter;
    binn value;
    size_t i = 0;

    binn_list_foreach(fields, value)
    {
        if (i > table->size)
        {
            MDV_LOGE("unbinn_table failed");
            mdv_free(table);
            return 0;
        }

        mdv_field *field = table->fields + i;
        char *field_name = 0;

        if (!binn_object_get_uint32(&value, "T", &field->type)
            || !binn_object_get_uint32(&value, "L", &field->limit)
            || !binn_object_get_str(&value, "N", &field_name))
        {
            MDV_LOGE("unbinn_table failed");
            mdv_free(table);
            return 0;
        }

        field->name.size = strlen(field_name) + 1;
        field->name.ptr = buff;
        buff += field->name.size;
        memcpy(field->name.ptr, field_name, field->name.size);

        ++i;
    }

    if (buff - (char*)table != size)
        MDV_LOGE("memory is corrupted: %p, %zu != %u", table, buff - (char*)table, size);

    return table;
}


static bool binn_add_to_list(binn *list, mdv_field_type type, void const *data)
{
    switch(type)
    {
        case MDV_FLD_TYPE_BOOL:     return binn_list_add_bool(list, *(bool const*)data);         break;
        case MDV_FLD_TYPE_CHAR:     return binn_list_add_int8(list, *(char const*)data);         break;
        case MDV_FLD_TYPE_BYTE:     return binn_list_add_int8(list, *(int8_t const*)data);       break;
        case MDV_FLD_TYPE_INT8:     return binn_list_add_int8(list, *(int8_t const*)data);       break;
        case MDV_FLD_TYPE_UINT8:    return binn_list_add_uint8(list, *(uint8_t const*)data);     break;
        case MDV_FLD_TYPE_INT16:    return binn_list_add_int16(list, *(int16_t const*)data);     break;
        case MDV_FLD_TYPE_UINT16:   return binn_list_add_uint16(list, *(uint16_t const*)data);   break;
        case MDV_FLD_TYPE_INT32:    return binn_list_add_int32(list, *(int32_t const*)data);     break;
        case MDV_FLD_TYPE_UINT32:   return binn_list_add_uint32(list, *(uint32_t const*)data);   break;
        case MDV_FLD_TYPE_INT64:    return binn_list_add_int64(list, *(int64_t const*)data);     break;
        case MDV_FLD_TYPE_UINT64:   return binn_list_add_uint64(list, *(uint64_t const*)data);   break;
        case MDV_FLD_TYPE_FLOAT:    return binn_list_add_float(list, *(float const*)data);       break;
        case MDV_FLD_TYPE_DOUBLE:   return binn_list_add_double(list, *(double const*)data);     break;
    }
    MDV_LOGE("Unknown field type: %u", type);
    return false;
}


static binn *binn_row(mdv_field const *fields, mdv_row_base const *row)
{
    binn *list = binn_list();
    if (!list)
    {
        MDV_LOGE("binn_row failed");
        return 0;
    }

    for(uint32_t i = 0; i < row->size; ++i)
    {
        uint32_t const field_type_size = mdv_field_type_size(fields[i].type);

        if(!field_type_size)
        {
            MDV_LOGE("binn_row failed. Invalid field type size.");
            binn_free(list);
            return 0;
        }

        if(row->fields[i].size % field_type_size)
        {
            MDV_LOGE("binn_row failed. Invalid field size.");
            binn_free(list);
            return 0;
        }

        uint32_t const arr_size = row->fields[i].size / field_type_size;

        if (fields[i].limit && fields[i].limit < arr_size)
        {
            MDV_LOGE("binn_row failed. Field is too long.");
            binn_free(list);
            return 0;
        }

        BOOL res = false;

        if(fields[i].limit == 1)
            res = binn_add_to_list(list, fields[i].type, row->fields[i].ptr);
        else
        {
            binn *field_items = binn_list();
            if (!field_items)
            {
                MDV_LOGE("binn_row failed");
                binn_free(list);
                return 0;
            }

            for(uint32_t j = 0; res && j < arr_size; ++j)
                res = binn_add_to_list(field_items, fields[i].type, row->fields[i].ptr + j * field_type_size);

            if (res)
                res = binn_list_add_list(list, field_items);

            binn_free(field_items);
        }

        if(!res)
        {
            MDV_LOGE("binn_row failed.");
            binn_free(list);
            return 0;
        }
    }

    return list;
}


mdv_table_storage mdv_table_create(mdv_storage *metainf_storage, mdv_table_base const *table)
{
    mdv_table_storage table_storage = {};
/*
    mdv_rollbacker(8) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_uuid_str(table_uuid);
    mdv_uuid_to_str(&table->uuid, &table_uuid);

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
        mdv_data const k = { table_uuid.size, table_uuid.ptr };
        mdv_data const v = { binn_size(obj),   binn_ptr(obj) };

        if (!mdv_map_put_unique(&metainf_map, &metainf_transaction, &k, &v))
        {
            MDV_LOGE("Table uuid '%s' isn't unique", table_uuid.ptr);
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
    path = mdv_str_pcat(mpool, path, dir_delimeter, table_uuid);

    if (mdv_str_empty(path))
    {
        MDV_LOGE("Path '%s' is too long.", MDV_CONFIG.storage.path.ptr);
        mdv_rollback(rollbacker);
        return table_storage;
    }

    // Create subdirectory for table
    if (!mdv_mkdir(path.ptr))
    {
        MDV_LOGE("Directory for table '%s' wasn't created", table_uuid.ptr);
        mdv_rollback(rollbacker);
        return table_storage;
    }

    mdv_rollbacker_push(rollbacker, mdv_rmdir, path.ptr);

    // Create table storage
    table_storage.table = mdv_table_clone(table);
    table_storage.data = mdv_storage_open(path.ptr, MDV_STRG_DATA, MDV_STRG_DATA_MAPS, MDV_STRG_NOSUBDIR);
    table_storage.log.add = mdv_storage_open(path.ptr, MDV_STRG_ADD_SET, MDV_STRG_ADD_SET_MAPS, MDV_STRG_NOSUBDIR);
    table_storage.log.rem = mdv_storage_open(path.ptr, MDV_STRG_REM_SET, MDV_STRG_REM_SET_MAPS, MDV_STRG_NOSUBDIR);

    mdv_rollbacker_push(rollbacker, mdv_table_close, &table_storage);

    if (!table_storage.table
        || !table_storage.data
        || !table_storage.log.add
        || !table_storage.log.rem)
    {
        MDV_LOGE("Srorage for table '%s' wasn't created", table_uuid.ptr);
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
        binn *obj = binn_table(table);
        if (!obj)
        {
            mdv_rollback(rollbacker);
            return table_storage;
        }

        // Save field information
        mdv_data const k = { 1, "T" };
        mdv_data const v = { binn_size(obj),   binn_ptr(obj) };

        if (!mdv_map_put_unique(&table_map, &table_transaction, &k, &v))
        {
            MDV_LOGE("Table '%s' isn't unique", table->name.ptr);
            mdv_rollback(rollbacker);
            binn_free(obj);
            return table_storage;
        }

        binn_free(obj);
    }

    if (!mdv_transaction_commit(&table_transaction))
    {
        mdv_rollback(rollbacker);
        return table_storage;
    }

    mdv_map_close(&table_map);

    if (!mdv_transaction_commit(&metainf_transaction))
    {
        mdv_rollback(rollbacker);
        return table_storage;
    }
    mdv_map_close(&metainf_map);

    MDV_LOGI("Table '%s' with uuid '%s' created", table->name.ptr, table_uuid.ptr);
*/
    return table_storage;
}


mdv_table_storage mdv_table_open(mdv_uuid const *uuid)
{
    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_table_storage table_storage = {};

    mdv_uuid_str(table_uuid);
    mdv_uuid_to_str(uuid, &table_uuid);

    // Build table subdirectory
    mdv_stack(char, MDV_PATH_MAX) mpool;
    mdv_stack_clear(mpool);

    static mdv_string const dir_delimeter = mdv_str_static("/");
    mdv_string path = mdv_str_pdup(mpool, MDV_CONFIG.storage.path.ptr);
    path = mdv_str_pcat(mpool, path, dir_delimeter, table_uuid);

    if (mdv_str_empty(path))
    {
        MDV_LOGE("Path '%s' is too long.", MDV_CONFIG.storage.path.ptr);
        return table_storage;
    }

    // Open table storages
    table_storage.data = mdv_storage_open(path.ptr, MDV_STRG_DATA, MDV_STRG_DATA_MAPS, MDV_STRG_NOSUBDIR);
    table_storage.log.add = mdv_storage_open(path.ptr, MDV_STRG_ADD_SET, MDV_STRG_ADD_SET_MAPS, MDV_STRG_NOSUBDIR);
    table_storage.log.rem = mdv_storage_open(path.ptr, MDV_STRG_REM_SET, MDV_STRG_REM_SET_MAPS, MDV_STRG_NOSUBDIR);

    if (!table_storage.data
        || !table_storage.log.add
        || !table_storage.log.rem)
    {
        MDV_LOGE("Srorage for table '%s' wasn't opened", table_uuid.ptr);
        mdv_table_close(&table_storage);
        return table_storage;
    }

    mdv_rollbacker_push(rollbacker, mdv_table_close, &table_storage);

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
    mdv_map table_map = mdv_map_open(&table_transaction, MDV_MAP_METAINF, 0);
    if (!mdv_map_ok(table_map))
    {
        MDV_LOGE("Table metainf storage table '%s' not opened", MDV_MAP_METAINF);
        mdv_rollback(rollbacker);
        return table_storage;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &table_map);

    // Read table information
    mdv_data const k = { 1, "T" };
    mdv_data v = {};
    if (!mdv_map_get(&table_map, &table_transaction, &k, &v))
    {
        MDV_LOGE("Table metainf storage table '%s' not read", MDV_MAP_METAINF);
        mdv_rollback(rollbacker);
        return table_storage;
    }

    binn *obj = binn_open(v.ptr);
    if (!obj)
    {
        MDV_LOGE("Table '%s' information wasn't read", table_uuid.ptr);
        mdv_rollback(rollbacker);
        return table_storage;
    }

    table_storage.table = unbinn_table(obj);

    binn_free(obj);

    if (!table_storage.table)
    {
        MDV_LOGE("Table '%s' information wasn't read", table_uuid.ptr);
        mdv_rollback(rollbacker);
        return table_storage;
    }

    mdv_transaction_abort(&table_transaction);
    mdv_map_close(&table_map);

    MDV_LOGI("Table with uuid '%s' opened", table_uuid.ptr);

    return table_storage;
}


void mdv_table_close(mdv_table_storage *storage)
{
    if (storage->table)
        mdv_free(storage->table);
    mdv_storage_release(storage->data);
    mdv_storage_release(storage->log.add);
    mdv_storage_release(storage->log.rem);
    storage->table = 0;
    storage->data = 0;
    storage->log.add = 0;
    storage->log.rem = 0;
}


bool mdv_table_drop(mdv_storage *metainf_storage, mdv_uuid const *uuid)
{
/*
    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_uuid_str(table_uuid);
    mdv_uuid_to_str(uuid, &table_uuid);

    // Start metainf transaction
    mdv_transaction metainf_transaction = mdv_transaction_start(metainf_storage);
    if (!mdv_transaction_ok(metainf_transaction))
    {
        MDV_LOGE("Metainf storage transaction not started");
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &metainf_transaction);

    // Open tables list
    mdv_map metainf_map = mdv_map_open(&metainf_transaction, MDV_MAP_TABLES, 0);
    if (!mdv_map_ok(metainf_map))
    {
        MDV_LOGE("Metainf storage table '%s' not created", MDV_MAP_TABLES);
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &metainf_map);

    // Build table subdirectory
    mdv_stack(char, MDV_PATH_MAX) mpool;
    mdv_stack_clear(mpool);

    static mdv_string const dir_delimeter = mdv_str_static("/");
    mdv_string path = mdv_str_pdup(mpool, MDV_CONFIG.storage.path.ptr);
    path = mdv_str_pcat(mpool, path, dir_delimeter, table_uuid);

    if (mdv_str_empty(path))
    {
        MDV_LOGE("Path '%s' is too long.", MDV_CONFIG.storage.path.ptr);
        mdv_rollback(rollbacker);
        return false;
    }

    // Remove short table information
    {
        mdv_data const k = { table_uuid.size, table_uuid.ptr };

        if (!mdv_map_del(&metainf_map, &metainf_transaction, &k, 0))
        {
            MDV_LOGE("Table '%s' wasn't dropped", table_uuid.ptr);
            mdv_rollback(rollbacker);
            return false;
        }
    }

    if (!mdv_transaction_commit(&metainf_transaction))
    {
        mdv_rollback(rollbacker);
        return false;
    }
    mdv_map_close(&metainf_map);

    MDV_LOGI("Table with uuid '%s' dropped", table_uuid.ptr);

    // Remove table storage directories
    if (!mdv_rmdir(path.ptr))
    {
        MDV_LOGE(">>>> Directory for table '%s' wasn't deleted. Delete it manually!!!", table_uuid.ptr);
        return true;
    }
*/
    return true;
}


bool mdv_table_insert(mdv_table_storage *storage, size_t count, mdv_row_base const **rows)
{
    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(storage->data);
    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("Rows storage transaction not started");
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open rows table
    mdv_map map = mdv_map_open(&transaction, MDV_MAP_ROWS, MDV_MAP_CREATE | MDV_MAP_INTEGERKEY);
    if (!mdv_map_ok(map))
    {
        MDV_LOGE("Rows storage table '%s' not opened", MDV_MAP_ROWS);
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &map);

    for(size_t i = 0; i < count; ++i)
    {
        mdv_row_base const *row = rows[i];

        binn *obj = binn_row(storage->table->fields, row);
        if(!obj)
        {
            MDV_LOGE("Row serialization failed.");
            mdv_rollback(rollbacker);
            return false;
        }

        mdv_data k = { sizeof(row->id), (void*)&row->id };
        mdv_data v = { binn_size(obj), binn_ptr(obj) };

        if (!mdv_map_put_unique(&map, &transaction, &k, &v))
        {
            MDV_LOGE("Row insertion failed.");
            mdv_rollback(rollbacker);
            binn_free(obj);
            return false;
        }

        binn_free(obj);
    }

    if (!mdv_transaction_commit(&transaction))
    {
        MDV_LOGE("Row insertion transaction failed.");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_map_close(&map);

    return true;
}

