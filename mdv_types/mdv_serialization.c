#include "mdv_serialization.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <mdv_rollbacker.h>
#include <assert.h>
#include <limits.h>


static bool binn_field(mdv_field const *field, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_field failed");
        return false;
    }

    if (0
        || !binn_object_set_uint32(obj, "T", (unsigned int)field->type)
        || !binn_object_set_uint32(obj, "L", (unsigned int)field->limit)
        || !binn_object_set_str(obj, "N", (char *)field->name))
    {
        MDV_LOGE("binn_field failed");
        binn_free(obj);
        return false;
    }

    return true;
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


static bool binn_get(binn *val, mdv_field_type type, void *data)
{
    bool ret = false;

    switch(type)
    {
        case MDV_FLD_TYPE_BOOL:
        {
            BOOL bool_val = 0;
            ret = binn_get_bool(val, &bool_val);
            *(bool*)data = bool_val != 0;
            break;
        }

        case MDV_FLD_TYPE_CHAR:
        {
            int int_val = 0;
            ret = binn_get_int32(val, &int_val);
            *(char*)data = (char)int_val;
            break;
        }

        case MDV_FLD_TYPE_BYTE:
        case MDV_FLD_TYPE_INT8:
        {
            int int_val = 0;
            ret = binn_get_int32(val, &int_val);
            *(int8_t*)data = (int8_t)int_val;
            break;
        }

        case MDV_FLD_TYPE_UINT8:
        {
            int int_val = 0;
            ret = binn_get_int32(val, &int_val);
            *(uint8_t*)data = (uint8_t)int_val;
            break;
        }

        case MDV_FLD_TYPE_INT16:
        {
            int int_val = 0;
            ret = binn_get_int32(val, &int_val);
            *(int16_t*)data = (int16_t)int_val;
            break;
        }

        case MDV_FLD_TYPE_UINT16:
        {
            int int_val = 0;
            ret = binn_get_int32(val, &int_val);
            *(uint16_t*)data = (uint16_t)int_val;
            break;
        }

        case MDV_FLD_TYPE_INT32:
        {
            int int_val = 0;
            ret = binn_get_int32(val, &int_val);
            *(int32_t*)data = (int32_t)int_val;
            break;
        }

        case MDV_FLD_TYPE_UINT32:
        {
            int64 int64_val = 0;
            ret = binn_get_int64(val, &int64_val);
            *(uint32_t*)data = (uint32_t)int64_val;
            break;
        }

        case MDV_FLD_TYPE_INT64:
        {
            int64 int64_val = 0;
            ret = binn_get_int64(val, &int64_val);
            *(int64_t*)data = (int64_t)int64_val;
            break;
        }

        case MDV_FLD_TYPE_UINT64:
        {
            int64 int64_val = 0;
            ret = binn_get_int64(val, &int64_val);
            *(uint64_t*)data = (uint64_t)int64_val;
            break;
        }

        case MDV_FLD_TYPE_FLOAT:
        {
            double double_val = 0;
            ret = binn_get_double(val, &double_val);
            *(float*)data = (float)double_val;
            break;
        }

        case MDV_FLD_TYPE_DOUBLE:
        {
            ret = binn_get_double(val, data);
            break;
        }
    }

    if (!ret)
        MDV_LOGE("Unknown field type: %u", type);

    return ret;
}


bool mdv_binn_table_desc(mdv_table_desc const *table, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_table_desc failed");
        return false;
    }

    if (0
        || !binn_object_set_str(obj, "N", (char*)table->name)
        || !binn_object_set_uint32(obj, "S", table->size))
    {
        MDV_LOGE("binn_table_desc failed");
        binn_free(obj);
        return false;
    }

    binn fields;

    if (!binn_create_list(&fields))
    {
        MDV_LOGE("binn_table_desc failed");
        binn_free(obj);
        return false;
    }

    for(uint32_t i = 0; i < table->size; ++i)
    {
        binn field;
        if(0
           || !binn_field(table->fields + i, &field)
           || !binn_list_add_object(&fields, &field))
        {
            binn_free(&field);
            binn_free(&fields);
            binn_free(obj);
            return false;
        }
        binn_free(&field);
    }

    if (!binn_object_set_list(obj, "F", &fields))
    {
        binn_free(&fields);
        binn_free(obj);
        return false;
    }

    binn_free(&fields);

    return true;
}


mdv_table_desc * mdv_unbinn_table_desc(binn const *obj)
{
    char *name = 0;
    uint32_t fields_count = 0;

    binn *binn_fields = 0;

    if (0
        || !binn_object_get_str((void*)obj, "N", &name)
        || !binn_object_get_uint32((void*)obj, "S", &fields_count)
        || !binn_object_get_list((void*)obj, "F", (void**)&binn_fields))
    {
        MDV_LOGE("unbinn_table_desc failed");
        return 0;
    }

    size_t const table_name_size = strlen(name) + 1;

    binn_iter iter = {};
    binn value = {};

    // Calculate size
    uint32_t size = sizeof(mdv_table_desc) + fields_count * sizeof(mdv_field)
                    + table_name_size;

    binn_list_foreach(binn_fields, value)
    {
        char *field_name = 0;

        if (!binn_object_get_str(&value, "N", &field_name))
        {
            MDV_LOGE("unbinn_table_desc failed");
            return 0;
        }

        size += strlen(field_name) + 1;
    }

    // Allocate memory for table desc
    mdv_table_desc *table = mdv_alloc(size);

    if (!table)
    {
        MDV_LOGE("unbinn_table failed");
        return 0;
    }

    table->size = fields_count;

    mdv_field *fields = (mdv_field *)(table + 1);

    table->fields = fields;

    char *buff = (char *)(fields + table->size);

    memcpy(buff, name, table_name_size);
    table->name = buff;
    buff += table_name_size;

    size_t i = 0;

    binn_list_foreach(binn_fields, value)
    {
        if (i > table->size)
        {
            MDV_LOGE("unbinn_table_desc failed");
            mdv_free(table);
            return 0;
        }

        mdv_field *field = fields + i;
        char *field_name = 0;

        if (!binn_object_get_uint32(&value, "T", &field->type)
            || !binn_object_get_uint32(&value, "L", &field->limit)
            || !binn_object_get_str(&value, "N", &field_name))
        {
            MDV_LOGE("unbinn_table_desc failed");
            mdv_free(table);
            return 0;
        }

        size_t const field_name_size = strlen(field_name) + 1;
        memcpy(buff, field_name, field_name_size);
        field->name = buff;
        buff += field_name_size;

        ++i;
    }

    if (buff - (char*)table != size)
        MDV_LOGE("memory corrupted: %p, %zu != %u", table, buff - (char*)table, size);

    assert(buff - (char*)table <= size);

    return table;
}


bool mdv_binn_table(mdv_table const *table, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_table failed");
        return false;
    }

    binn desc;

    if (!mdv_binn_table_desc(mdv_table_description(table), &desc))
    {
        binn_free(obj);
        return false;
    }

    if (!mdv_binn_table_uuid(mdv_table_uuid(table), obj)
        || !binn_object_set_object(obj, "D", &desc))
    {
        MDV_LOGE("binn_table failed");
        binn_free(obj);
        binn_free(&desc);
        return false;
    }

    binn_free(&desc);

    return true;
}


mdv_table * mdv_unbinn_table(binn const *obj)
{
    mdv_uuid id;

    if (!mdv_unbinn_table_uuid(obj, &id))
    {
        MDV_LOGE("unbinn_table failed");
        return 0;
    }

    binn *desc_odj = 0;

    if (!binn_object_get_object((void*)obj, "D", (void**)&desc_odj))
    {
        MDV_LOGE("unbinn_table failed");
        return 0;
    }

    mdv_table_desc *desc = mdv_unbinn_table_desc(desc_odj);

    if (!desc)
    {
        MDV_LOGE("unbinn_table failed");
        return 0;
    }

    mdv_table *table = mdv_table_create(&id, desc);

    mdv_free(desc);

    if (!table)
    {
        MDV_LOGE("unbinn_table failed");
        return 0;
    }

    return table;
}


mdv_rowlist_entry * mdv_unbinn_table_as_row_slice(binn const        *obj,
                                                  mdv_bitset const  *mask)
{
    mdv_uuid uuid        = {};
    char *name           = 0;
    size_t name_size     = 0;
    uint32_t const cols  = 2;
    uint32_t fields_num  = 0;
    size_t row_size      = 0;

    // Calculate necessary space for row
    for(uint32_t n = 0; n < cols; ++n)
    {
        if (mask && !mdv_bitset_test(mask, n))
            continue;

        switch(n)
        {
            case 0:
            {
                if (!mdv_unbinn_table_uuid(obj, &uuid))
                {
                    MDV_LOGE("unbinn_table failed");
                    return 0;
                }

                row_size += sizeof(mdv_uuid);

                break;
            }

            case 1:
            {
                binn *desc_odj = 0;

                if (!binn_object_get_object((void*)obj, "D", (void**)&desc_odj))
                {
                    MDV_LOGE("unbinn_table failed");
                    return 0;
                }

                if (!binn_object_get_str((void*)desc_odj, "N", &name))
                {
                    MDV_LOGE("unbinn_table_desc failed");
                    return 0;
                }

                name_size = strlen(name) + 1;

                row_size += name_size;

                break;
            }

            default:
            {
                MDV_LOGE("Unknown field requested");
                return 0;
            }
        }

        ++fields_num;
    }

    if (!fields_num ||
        fields_num > cols)
    {
        MDV_LOGE("Serialized row contains invalid number of fields");
        return 0;
    }

    row_size += offsetof(mdv_rowlist_entry, data)
                + offsetof(mdv_row, fields)
                + sizeof(mdv_data) * fields_num;

    // Memory allocation for new row
    mdv_rowlist_entry *entry = mdv_alloc(row_size);

    if (!entry)
    {
        MDV_LOGE("No memory for new row");
        return 0;
    }

    mdv_row *row = &entry->data;

    char *dataspace = (char *)(row->fields + fields_num);

    size_t field_idx = 0;

    {
        row->fields[field_idx].ptr = dataspace;
        row->fields[field_idx].size = sizeof(mdv_uuid);
        memcpy(dataspace, &uuid, sizeof(mdv_uuid));
        ++field_idx;
        dataspace += sizeof(mdv_uuid);
    }

    {
        row->fields[field_idx].ptr = dataspace;
        row->fields[field_idx].size = name_size;
        memcpy(dataspace, name, name_size);
        ++field_idx;
        dataspace += name_size;
    }

    if (dataspace - (char*)entry != row_size)
        MDV_LOGE("memory corrupted: %p, %zu != %zu", entry, dataspace - (char*)entry, row_size);

    assert(dataspace - (char const*)entry == row_size);

    return entry;
}


bool mdv_binn_table_uuid(mdv_uuid const *uuid, binn *obj)
{
    return binn_object_set_uint64((void*)obj, "U0", uuid->u64[0])
           && binn_object_set_uint64((void*)obj, "U1", uuid->u64[1]);
}


bool mdv_unbinn_table_uuid(binn const *obj, mdv_uuid *uuid)
{
    if (0
        || !binn_object_get_uint64((void*)obj, "U0", (uint64 *)&uuid->u64[0])
        || !binn_object_get_uint64((void*)obj, "U1", (uint64 *)&uuid->u64[1]))
    {
        MDV_LOGE("unbinn_table failed");
        return false;
    }

    return true;
}


bool mdv_binn_row(mdv_row const *row, mdv_table_desc const *table_desc, binn *list)
{
    if (!binn_create_list(list))
    {
        MDV_LOGE("binn_row failed");
        return false;
    }

    mdv_field const *fields = table_desc->fields;

    for(uint32_t i = 0; i < table_desc->size; ++i)
    {
        uint32_t const field_type_size = mdv_field_type_size(fields[i].type);

        if(!field_type_size)
        {
            MDV_LOGE("binn_row failed. Invalid field type size.");
            binn_free(list);
            return false;
        }

        if(row->fields[i].size % field_type_size)
        {
            MDV_LOGE("binn_row failed. Invalid field size.");
            binn_free(list);
            return false;
        }

        uint32_t const arr_size = row->fields[i].size / field_type_size;

        if (fields[i].limit && fields[i].limit < arr_size)
        {
            MDV_LOGE("binn_row failed. Field is too long.");
            binn_free(list);
            return false;
        }

        BOOL res = true;

        if(fields[i].limit == 1)
            res = binn_add_to_list(list, fields[i].type, row->fields[i].ptr);
        else if (field_type_size == 1)
            res = binn_list_add_blob(list, row->fields[i].ptr, arr_size);
        else
        {
            binn *field_items = binn_list();

            if (!field_items)
            {
                MDV_LOGE("binn_row failed");
                binn_free(list);
                return false;
            }

            for(uint32_t j = 0; res && j < arr_size; ++j)
                res = binn_add_to_list(field_items, fields[i].type, (char const *)row->fields[i].ptr + j * field_type_size);

            if (res)
                res = binn_list_add_list(list, field_items);

            binn_free(field_items);
        }

        if(!res)
        {
            MDV_LOGE("binn_row failed.");
            binn_free(list);
            return false;
        }
    }

    return true;
}


static size_t mdv_calc_row_size(binn const           *list,
                                mdv_table_desc const *table_desc,
                                uint32_t             *fields_count,
                                mdv_bitset const     *mask)
{
    *fields_count = 0;

    binn_iter iter = {};
    binn value = {};

    uint32_t const cols = table_desc->size;
    mdv_field const *fields = table_desc->fields;

    uint32_t n = 0;

    size_t row_size = 0;

    // Calculate necessary space for row
    binn_list_foreach((void*)list, value)
    {
        if (mask && !mdv_bitset_test(mask, n))
        {
            ++n;
            continue;
        }

        uint32_t const field_type_size = mdv_field_type_size(fields[n].type);

        if(fields[n].limit == 1)
            row_size += field_type_size;
        else if (field_type_size == 1)
        {
            int const blob_size = binn_size(&value);

            if (blob_size < 0)
            {
                MDV_LOGE("blob size is negative: %d", blob_size);
                return 0;
            }

            row_size += blob_size;
        }
        else
            row_size += field_type_size * mdv_binn_list_length(&value);

        ++n;
        ++*fields_count;
    }

    if (*fields_count > cols)
    {
        MDV_LOGE("Serialized row contains invalid number of fields");
        return 0;
    }

    row_size += offsetof(mdv_rowlist_entry, data)
                + offsetof(mdv_row, fields)
                + sizeof(mdv_data) * *fields_count;

    return row_size;
}


mdv_rowlist_entry * mdv_unbinn_row(binn const *list, mdv_table_desc const *table_desc)
{
    return mdv_unbinn_row_slice(list, table_desc, 0);
}


mdv_rowlist_entry * mdv_unbinn_row_slice(binn const *list,
                                         mdv_table_desc const *table_desc,
                                         mdv_bitset const *mask)
{
    uint32_t const cols = table_desc->size;
    mdv_field const *fields = table_desc->fields;

    uint32_t fields_count = 0;

    // Calculate necessary space for row
    size_t const row_size = mdv_calc_row_size(list, table_desc, &fields_count, mask);

    if (!row_size)
        return 0;

    // Memory allocation for new row
    mdv_rowlist_entry *entry = mdv_alloc(row_size);

    if (!entry)
    {
        MDV_LOGE("No memory for new row");
        return 0;
    }

    mdv_row *row = &entry->data;

    char *dataspace = (char *)(row->fields + fields_count);

    uint32_t n = 0, field_idx = 0;

    binn_iter iter = {};
    binn value = {};

    // Deserialize row
    binn_list_foreach((void*)list, value)
    {
        if (mask && !mdv_bitset_test(mask, n))
        {
            ++n;
            continue;
        }

        uint32_t const field_type_size = mdv_field_type_size(fields[n].type);

        if(fields[n].limit == 1)
        {
            if (!binn_get(&value, fields[n].type, dataspace))
            {
                MDV_LOGE("unbinn_table failed");
                mdv_free(entry);
                return 0;
            }

            row->fields[field_idx].size = field_type_size;
            row->fields[field_idx].ptr = dataspace;
            dataspace += field_type_size;
        }
        else if (field_type_size == 1)
        {
            int const blob_size = binn_size(&value);
            memcpy(dataspace, binn_ptr(&value), blob_size);
            row->fields[field_idx].size = blob_size;
            row->fields[field_idx].ptr = dataspace;
            dataspace += blob_size;
        }
        else
        {
            binn_iter arr_iter = {};
            binn arr_value = {};
            uint32_t arr_len = 0;

            row->fields[field_idx].ptr = dataspace;

            binn_iter_init(&arr_iter, &value, BINN_LIST);

            while (binn_list_next(&arr_iter, &arr_value))
            {
                if (!binn_get(&arr_value, fields[n].type, dataspace))
                {
                    MDV_LOGE("unbinn_table failed");
                    mdv_free(row);
                    return 0;
                }

                ++arr_len;
                dataspace += field_type_size;
            }

            row->fields[field_idx].size = field_type_size * arr_len;
        }

        ++n;
        ++field_idx;
    }

    if (dataspace - (char*)entry != row_size)
        MDV_LOGE("memory corrupted: %p, %zu != %zu", entry, dataspace - (char*)entry, row_size);

    assert(dataspace - (char const*)entry == row_size);

    return entry;
}


bool mdv_binn_rowset(mdv_rowset *rowset, binn *list)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    if (!binn_create_list(list))
    {
        MDV_LOGE("binn_rowset failed");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, binn_free, list);

    mdv_enumerator *enumerator = mdv_rowset_enumerator(rowset);

    if (!enumerator)
    {
        MDV_LOGE("binn_rowset failed");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_enumerator_release, enumerator);

    mdv_table *table = mdv_rowset_table(rowset);

    mdv_rollbacker_push(rollbacker, mdv_table_release, table);

    mdv_table_desc const *table_desc = mdv_table_description(table);

    while(mdv_enumerator_next(enumerator) == MDV_OK)
    {
        mdv_row *row = mdv_enumerator_current(enumerator);

        binn fields;

        if (mdv_binn_row(row, table_desc, &fields))
        {
            if (!binn_list_add_list(list, &fields))
            {
                binn_free(&fields);
                MDV_LOGE("binn_rowset failed");
                mdv_rollback(rollbacker);
                return false;
            }

            binn_free(&fields);
        }
        else
        {
            MDV_LOGE("binn_rowset failed");
            mdv_rollback(rollbacker);
            return false;
        }
    }

    mdv_table_release(table);
    mdv_enumerator_release(enumerator);
    mdv_rollbacker_free(rollbacker);

    return true;
}


mdv_rowset * mdv_unbinn_rowset(binn const *list, mdv_table *table)
{
    mdv_rowset *rowset = mdv_rowset_create(table);

    if (!rowset || !table)
    {
        MDV_LOGE("unbinn_rowset failed");
        return 0;
    }

    mdv_table_desc const *table_desc = mdv_table_description(table);

    binn_iter iter = {};
    binn value = {};

    binn_list_foreach((void*)list, value)
    {
        mdv_rowlist_entry *entry = mdv_unbinn_row(&value, table_desc);

        if (!entry)
        {
            MDV_LOGE("unbinn_rowset failed");
            mdv_rowset_release(rowset);
            return 0;
        }

        mdv_rowset_emplace(rowset, entry);
    }

    return rowset;
}


bool mdv_topology_serialize(mdv_topology *topology, binn *obj)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(6);

    binn nodes;
    binn links;

    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_topology failed");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, binn_free, obj);

    if (!binn_create_list(&nodes))
    {
        MDV_LOGE("binn_topology failed");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &nodes);

    if (!binn_create_list(&links))
    {
        MDV_LOGE("binn_topology failed");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &links);

    mdv_vector *toponodes = mdv_topology_nodes(topology);
    mdv_vector *topolinks = mdv_topology_links(topology);
    mdv_vector *topoextradata = mdv_topology_extradata(topology);

    mdv_rollbacker_push(rollbacker, mdv_vector_release, toponodes);
    mdv_rollbacker_push(rollbacker, mdv_vector_release, topolinks);
    mdv_rollbacker_push(rollbacker, mdv_vector_release, topoextradata);

    mdv_vector_foreach(toponodes, mdv_toponode, toponode)
    {
        binn node;

        if (!binn_create_object(&node))
        {
            MDV_LOGE("binn_topology failed");
            mdv_rollback(rollbacker);
            return false;
        }

        if (0
            || !binn_object_set_uint32(&node, "ID", toponode->id)
            || !binn_object_set_uint64(&node, "U1", toponode->uuid.u64[0])
            || !binn_object_set_uint64(&node, "U2", toponode->uuid.u64[1])
            || !binn_object_set_str(&node, "A", (char*)toponode->addr)
            || !binn_list_add_object(&nodes, &node))
        {
            MDV_LOGE("binn_topology failed");
            binn_free(&node);
            mdv_rollback(rollbacker);
            return false;
        }

        binn_free(&node);
    }

    mdv_vector_foreach(topolinks, mdv_topolink, topolink)
    {
        binn link;
        uint8_t tmp[64];

        if (!binn_create(&link, BINN_OBJECT, sizeof tmp, tmp))
        {
            MDV_LOGE("binn_topology failed");
            mdv_rollback(rollbacker);
            return false;
        }

        if (0
            || !binn_object_set_uint32(&link, "U1", topolink->node[0])
            || !binn_object_set_uint32(&link, "U2", topolink->node[1])
            || !binn_object_set_uint32(&link, "W", topolink->weight)
            || !binn_list_add_object(&links, &link))
        {
            MDV_LOGE("binn_topology failed");
            binn_free(&link);
            mdv_rollback(rollbacker);
            return false;
        }

        binn_free(&link);
    }

    if (0
        || !binn_object_set_uint64(obj, "NC", mdv_vector_size(toponodes))
        || !binn_object_set_uint64(obj, "LC", mdv_vector_size(topolinks))
        || !binn_object_set_uint64(obj, "ES", mdv_vector_size(topoextradata))
        || (mdv_vector_empty(toponodes) ? false : !binn_object_set_list(obj, "N", &nodes))
        || (mdv_vector_empty(topolinks) ? false : !binn_object_set_list(obj, "L", &links)))
    {
        MDV_LOGE("binn_topology failed");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_vector_release(toponodes);
    mdv_vector_release(topolinks);
    mdv_vector_release(topoextradata);

    binn_free(&nodes);
    binn_free(&links);

    mdv_rollbacker_free(rollbacker);

    return true;
}


mdv_topology * mdv_topology_deserialize(binn const *obj)
{
    uint64   nodes_count = 0;
    uint64   links_count = 0;
    uint64   extradata_size = 0;
    binn    *nodes = 0;
    binn    *links = 0;

    if (0
        || !binn_object_get_uint64((void*)obj, "NC", &nodes_count)
        || !binn_object_get_uint64((void*)obj, "LC", &links_count)
        || !binn_object_get_uint64((void*)obj, "ES", &extradata_size)
        || (nodes_count && !binn_object_get_list((void*)obj, "N", (void**)&nodes))
        || (links_count && !binn_object_get_list((void*)obj, "L", (void**)&links)))
    {
        MDV_LOGE("unbinn_topology failed");
        return 0;
    }

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_vector *toponodes = mdv_vector_create(nodes_count,
                                              sizeof(mdv_toponode),
                                              &mdv_default_allocator);

    if(!toponodes)
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, toponodes);

    mdv_vector *topolinks = mdv_vector_create(links_count,
                                              sizeof(mdv_topolink),
                                              &mdv_default_allocator);

    if(!topolinks)
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, topolinks);

    mdv_vector *extradata = mdv_vector_create(extradata_size,
                                              sizeof(char),
                                              &mdv_default_allocator);

    if(!extradata)
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, extradata);

    binn_iter iter = {};
    binn value = {};

    size_t i;

    // load nodes
    if (nodes_count)
    {
        i = 0;
        binn_list_foreach(nodes, value)
        {
            if (i++ > nodes_count)
            {
                MDV_LOGE("unbinn_topology failed");
                mdv_rollback(rollbacker);
                return 0;
            }

            mdv_toponode node;

            char *addr = 0;

            if (0
                || !binn_object_get_uint32(&value, "ID", &node.id)
                || !binn_object_get_uint64(&value, "U1", (uint64*)&node.uuid.u64[0])
                || !binn_object_get_uint64(&value, "U2", (uint64*)&node.uuid.u64[1])
                || !binn_object_get_str(&value, "A", &addr))
            {
                MDV_LOGE("unbinn_topology failed");
                mdv_rollback(rollbacker);
                return 0;
            }

            node.addr = mdv_vector_append(extradata, addr, strlen(addr) + 1);

            mdv_vector_push_back(toponodes, &node);
        }
    }

    // load links
    if (links_count)
    {
        i = 0;
        binn_list_foreach(links, value)
        {
            if (i++ > links_count)
            {
                MDV_LOGE("unbinn_topology failed");
                mdv_rollback(rollbacker);
                return 0;
            }

            mdv_topolink link;

            if (0
                || !binn_object_get_uint32(&value, "U1", link.node + 0)
                || !binn_object_get_uint32(&value, "U2", link.node + 1)
                || !binn_object_get_uint32(&value, "W", &link.weight)
                || link.node[0] >= nodes_count
                || link.node[1] >= nodes_count)
            {
                MDV_LOGE("unbinn_topology failed");
                mdv_rollback(rollbacker);
                return 0;
            }

            mdv_vector_push_back(topolinks, &link);
        }
    }

    mdv_topology *topology = mdv_topology_create(toponodes, topolinks, extradata);

    mdv_rollback(rollbacker);

    return topology;
}


bool mdv_binn_uuid(mdv_uuid const *uuid, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_uuid failed");
        return false;
    }

    if (0
        || !binn_object_set_uint64(obj, "U1", uuid->u64[0])
        || !binn_object_set_uint64(obj, "U2", uuid->u64[1]))
    {
        MDV_LOGE("binn_uuid failed");
        binn_free(obj);
        return false;
    }

    return true;
}


bool mdv_unbinn_uuid(binn *obj, mdv_uuid *uuid)
{
    if (0
        || !binn_object_get_uint64(obj, "U1", (uint64*)&uuid->u64[0])
        || !binn_object_get_uint64(obj, "U2", (uint64*)&uuid->u64[1]))
    {
        MDV_LOGE("unbinn_uuid failed");
        return false;
    }

    return true;
}


bool mdv_binn_bitset(mdv_bitset const *bitset, binn *obj)
{
    if (!binn_create_list(obj))
    {
        MDV_LOGE("binn_bitset failed");
        return false;
    }

    size_t const capacity = mdv_bitset_capacity(bitset);
    int32_t const *data = (int32_t const *)mdv_bitset_data(bitset);

    for(size_t i = 0; i < capacity / (MDV_BITSET_ALIGNMENT * CHAR_BIT); ++i)
    {
        if (!binn_list_add_int32(obj, data[i]))
        {
            MDV_LOGE("binn_bitset failed");
            binn_free(obj);
            return false;
        }
    }

    return true;
}


mdv_bitset * mdv_unbinn_bitset(binn const *obj)
{
    size_t const list_len = mdv_binn_list_length(obj);
    size_t const capacity = list_len * MDV_BITSET_ALIGNMENT * CHAR_BIT;

    mdv_bitset *bitset = mdv_bitset_create(capacity, &mdv_default_allocator);

    if (!bitset)
    {
        MDV_LOGE("unbinn_bitset failed. No memory.");
        return 0;
    }

    int32_t *data = (int32_t *)mdv_bitset_data(bitset);

    binn_iter iter;
    binn value;
    uint32_t n = 0;

    binn_list_foreach((binn*)obj, value)
    {
        if (!binn_get_int32(&value, data + n))
        {
            MDV_LOGE("unbinn_bitset failed");
            mdv_bitset_release(bitset);
            return 0;
        }

        ++n;
    }

    return bitset;
}
