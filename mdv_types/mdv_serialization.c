#include "mdv_serialization.h"
#include <mdv_log.h>
#include <mdv_alloc.h>


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


binn * mdv_binn_table(mdv_table_base const *table)
{
    // Calculate size
    uint32_t size = offsetof(mdv_table_base, fields) + table->size * sizeof(mdv_field);
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


mdv_table_base * mdv_unbinn_table(binn *obj)
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


binn * mdv_binn_row(mdv_field const *fields, mdv_row_base const *row)
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


static uint32_t binn_list_size(binn *obj)
{
    binn_iter iter;
    binn value;
    uint32_t n = 0;
    binn_list_foreach(obj, value)
        ++n;
    return n;
}


mdv_row_base * mdv_unbinn_row(binn *obj, mdv_field const *fields)
{
    binn_iter iter;
    binn value;

    uint32_t size = offsetof(mdv_row_base, fields);
    uint32_t fields_count = 0;

    // Calculate necessary space for row
    binn_list_foreach(obj, value)
    {
        uint32_t const field_type_size = mdv_field_type_size(fields[fields_count].type);

        if(fields[fields_count].limit == 1)
            size += sizeof(mdv_data) + field_type_size;
        else if (field_type_size == 1)
        {
            int const blob_size = binn_size(&value);
            if (blob_size < 0)
            {
                MDV_LOGE("blob size is negative: %d", blob_size);
                return 0;
            }
            size += sizeof(mdv_data) + blob_size;
        }
        else
            size += sizeof(mdv_data) + field_type_size * binn_list_size(&value);

        ++fields_count;
    }

    mdv_row_base *row = (mdv_row_base *)mdv_alloc(size);
    if (!row)
    {
        MDV_LOGE("unbinn_table failed");
        return 0;
    }

    row->size = fields_count;

    fields_count = 0;

    char *buff = (char *)(row->fields + row->size);

    // Deserialize row
    binn_list_foreach(obj, value)
    {
        uint32_t const field_type_size = mdv_field_type_size(fields[fields_count].type);

        if(fields[fields_count].limit == 1)
        {
            if (!binn_get(&value, fields[fields_count].type, buff))
            {
                MDV_LOGE("unbinn_table failed");
                mdv_free(row);
                return 0;
            }
            row->fields[fields_count].size = field_type_size;
            row->fields[fields_count].ptr = buff;
            buff++;

        }
        else if (field_type_size == 1)
        {
            int const blob_size = binn_size(&value);
            memcpy(buff, binn_ptr(&value), blob_size);
            row->fields[fields_count].size = blob_size;
            row->fields[fields_count].ptr = buff;
            buff += blob_size;
        }
        else
        {
            binn_iter arr_iter;
            binn arr_value;
            uint32_t arr_len = 0;

            row->fields[fields_count].ptr = buff;

            binn_list_foreach(&value, arr_value)
            {
                if (!binn_get(&arr_value, fields[fields_count].type, buff))
                {
                    MDV_LOGE("unbinn_table failed");
                    mdv_free(row);
                    return 0;
                }

                ++arr_len;
                buff += field_type_size;
            }

            row->fields[fields_count].size = field_type_size * arr_len;
        }

        ++fields_count;
    }

    if (buff - (char*)row != size)
        MDV_LOGE("memory is corrupted: %p, %zu != %u", row, buff - (char*)row, size);

    return row;
}