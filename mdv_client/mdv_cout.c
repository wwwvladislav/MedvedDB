#include "mdv_cout.h"
#include <stdio.h>
#include <inttypes.h>


static const size_t MDV_MAX_FIELD_WIDTH = 30;
static const uint32_t MDV_MAX_FIELD_LIMIT = 100500;


static size_t mdv_field_width(mdv_field const *field)
{
    uint32_t const limit = !field->limit
                            ? MDV_MAX_FIELD_LIMIT
                            : field->limit;

    size_t field_len = field->name.size;

    switch(field->type)
    {
        case MDV_FLD_TYPE_BOOL:     field_len = limit;              break;          // 01010110101
        case MDV_FLD_TYPE_CHAR:     field_len = limit;              break;          // Text
        case MDV_FLD_TYPE_BYTE:     field_len = 2 * limit;          break;          // 0102030405060708090A0B...
        case MDV_FLD_TYPE_INT8:     field_len = limit > 1
                                                    ? 5 * limit - 1
                                                    : 4 * limit;    break;          // -XXX,-XXX,-XXX
        case MDV_FLD_TYPE_UINT8:    field_len = limit > 1
                                                    ? 4 * limit - 1
                                                    : 3 * limit;    break;          // XXX,XXX,XXX

        case MDV_FLD_TYPE_INT16:    field_len = limit > 1
                                                    ? 7 * limit - 1
                                                    : 6 * limit;    break;          // -XXXXX,-XXXXX,-XXXXX
        case MDV_FLD_TYPE_UINT16:   field_len = limit > 1
                                                    ? 6 * limit - 1
                                                    : 5 * limit;    break;          // XXXXX,XXXXX,XXXXX
        case MDV_FLD_TYPE_INT32:    field_len = limit > 1
                                                    ? 12 * limit - 1
                                                    : 11 * limit;   break;          // -XXXXXXXXXX,-XXXXXXXXXX,-XXXXXXXXXX
        case MDV_FLD_TYPE_UINT32:   field_len = limit > 1
                                                    ? 11 * limit - 1
                                                    : 10 * limit;   break;          // XXXXXXXXXX,XXXXXXXXXX,XXXXXXXXXX
        case MDV_FLD_TYPE_INT64:    field_len = limit > 1
                                                    ? 21 * limit - 1
                                                    : 20 * limit;   break;          // -XXXXXXXXXXXXXXXXXXX,-XXXXXXXXXXXXXXXXXXX,-XXXXXXXXXXXXXXXXXXX
        case MDV_FLD_TYPE_UINT64:   field_len = limit > 1
                                                    ? 20 * limit - 1
                                                    : 19 * limit;   break;          // XXXXXXXXXXXXXXXXXXX,XXXXXXXXXXXXXXXXXXX,XXXXXXXXXXXXXXXXXXX
        case MDV_FLD_TYPE_FLOAT:    field_len = limit > 1
                                                    ? 12 * limit - 1
                                                    : 11 * limit;   break;          // -XXXXXXXXXX,-XXXXXXXXXX,-XXXXXXXXXX
        case MDV_FLD_TYPE_DOUBLE:   field_len = limit > 1
                                                    ? 21 * limit - 1
                                                    : 20 * limit;   break;          // -XXXXXXXXXXXXXXXXXXX,-XXXXXXXXXXXXXXXXXXX,-XXXXXXXXXXXXXXXXXXX
    }

    field_len = field_len > field->name.size
                    ? field_len
                    : field->name.size;

    field_len = field_len > MDV_MAX_FIELD_WIDTH
                    ? MDV_MAX_FIELD_WIDTH:
                    field_len;

    return field_len;
}


static void mdv_cout_spaces(size_t start, size_t end)
{
    for(size_t i = start; i < end; ++i)
        printf(" ");
}


static void mdv_cout_string(mdv_string const *str, size_t width)
{
    if(str->size > width)
        printf("%.*s~", (int)width - 1, str->ptr);
    else
    {
        int const ret = printf("%.*s", (int)str->size, str->ptr);
        if (ret >= 0)
            mdv_cout_spaces(ret, width);
    }
}


static void mdv_cout_bool(mdv_data const *data, mdv_field const *field)
{
    size_t const width = mdv_field_width(field);
    size_t const item_size = mdv_field_type_size(field->type);
    size_t const items_count = data->size / item_size;

    size_t len = 0;

    bool *ptr = data->ptr;

    for(size_t i = 0; i < items_count; ++i, ++ptr)
    {
        if (len + 1 > width)
        {
            printf("~");
            break;
        }

        printf(*ptr ? "1" : "0");
        ++len;
    }

    mdv_cout_spaces(len, width);
}


static void mdv_cout_char(mdv_data const *data, mdv_field const *field)
{
    size_t const width = mdv_field_width(field);

    mdv_string const str =
    {
        .size = data->size,
        .ptr = data->ptr
    };

    mdv_cout_string(&str, width);
}


static void mdv_cout_byte(mdv_data const *data, mdv_field const *field)
{
    size_t const width = mdv_field_width(field);
    size_t const item_size = mdv_field_type_size(field->type);
    size_t const items_count = data->size / item_size;

    size_t len = 0;

    uint8_t *ptr = data->ptr;

    for(size_t i = 0; i < items_count; ++i, ++ptr)
    {
        if (len + 2 > width)
        {
            printf("~");
            break;
        }

        printf("%02x", *ptr);

        len += 2;
    }

    mdv_cout_spaces(len, width);
}


static void mdv_cout_item(mdv_data const *data, mdv_field const *field, int (*outfn)(char *, size_t, void const *))
{
    size_t const width = mdv_field_width(field);
    size_t const item_size = mdv_field_type_size(field->type);
    size_t const items_count = data->size / item_size;

    size_t len = 0;

    uint8_t *ptr = data->ptr;

    char buff[64];

    for(size_t i = 0; i < items_count; ++i, ptr += item_size)
    {
        if (len + 1 > width)
        {
            printf("~");
            break;
        }

        if(i)
        {
            printf(",");
            ++len;
        }

        int ret = outfn(buff, sizeof buff, ptr);

        if (ret < 0)
        {
            printf("???");
            len += 3;
            break;
        }

        if (len + ret > width)
        {
            printf("~");
            break;
        }

        printf("%s", buff);

        len += ret;
    }

    mdv_cout_spaces(len, width);
}


static int mdv_cout_int8(char *buff, size_t size, void const *val)
{
    int ret = snprintf(buff, size, "%d", *(int8_t*)val);
    buff[size - 1] = 0;
    return ret;
}


static int mdv_cout_uint8(char *buff, size_t size, void const *val)
{
    int ret = snprintf(buff, size, "%u", *(uint8_t*)val);
    buff[size - 1] = 0;
    return ret;
}


static int mdv_cout_int16(char *buff, size_t size, void const *val)
{
    int ret = snprintf(buff, size, "%d", *(int16_t*)val);
    buff[size - 1] = 0;
    return ret;
}


static int mdv_cout_uint16(char *buff, size_t size, void const *val)
{
    int ret = snprintf(buff, size, "%u", *(uint16_t*)val);
    buff[size - 1] = 0;
    return ret;
}


static int mdv_cout_int32(char *buff, size_t size, void const *val)
{
    int ret = snprintf(buff, size, "%d", *(int32_t*)val);
    buff[size - 1] = 0;
    return ret;
}


static int mdv_cout_uint32(char *buff, size_t size, void const *val)
{
    int ret = snprintf(buff, size, "%u", *(uint32_t*)val);
    buff[size - 1] = 0;
    return ret;
}


static int mdv_cout_int64(char *buff, size_t size, void const *val)
{
    int ret = snprintf(buff, size, "%" PRId64, *(int64_t*)val);
    buff[size - 1] = 0;
    return ret;
}


static int mdv_cout_uint64(char *buff, size_t size, void const *val)
{
    int ret = snprintf(buff, size, "%" PRIu64, *(uint64_t*)val);
    buff[size - 1] = 0;
    return ret;
}


static int mdv_cout_float(char *buff, size_t size, void const *val)
{
    int ret = snprintf(buff, size, "%.2f", *(float*)val);
    buff[size - 1] = 0;
    return ret;
}


static int mdv_cout_double(char *buff, size_t size, void const *val)
{
    int ret = snprintf(buff, size, "%.4f", *(float*)val);
    buff[size - 1] = 0;
    return ret;
}


static void mdv_cout_table_header(mdv_table const *table)
{
    mdv_table_desc const *desc = mdv_table_description(table);

    printf("| ");

    for(uint32_t i = 0; i < desc->size; ++i)
    {
        if (i) printf(" | ");

        size_t const field_width = mdv_field_width(desc->fields + i);

        mdv_cout_string(&desc->fields[i].name, field_width);
    }
    printf(" |\n");
}


static void mdv_cout_table_separator(mdv_table const *table)
{
    mdv_table_desc const *desc = mdv_table_description(table);

    printf("+");

    for(uint32_t i = 0; i < desc->size; ++i)
    {
        if (i) printf("+");

        size_t const field_width = mdv_field_width(desc->fields + i);

        for(size_t j = 0; j < field_width + 2; ++j)
            printf("-");
    }

    printf("+\n");
}


static void mdv_cout_table_row(mdv_table const *table, mdv_row const *row)
{
    mdv_table_desc const *desc = mdv_table_description(table);

    printf("| ");

    for(uint32_t i = 0; i < desc->size; ++i)
    {
        if (i) printf(" | ");

        mdv_field const *field = desc->fields + i;

        switch(field->type)
        {
            case MDV_FLD_TYPE_BOOL:     mdv_cout_bool(row->fields + i, field);                  break;
            case MDV_FLD_TYPE_CHAR:     mdv_cout_char(row->fields + i, field);                  break;
            case MDV_FLD_TYPE_BYTE:     mdv_cout_byte(row->fields + i, field);                  break;
            case MDV_FLD_TYPE_INT8:     mdv_cout_item(row->fields + i, field, mdv_cout_int8);   break;
            case MDV_FLD_TYPE_UINT8:    mdv_cout_item(row->fields + i, field, mdv_cout_uint8);  break;
            case MDV_FLD_TYPE_INT16:    mdv_cout_item(row->fields + i, field, mdv_cout_int16);  break;
            case MDV_FLD_TYPE_UINT16:   mdv_cout_item(row->fields + i, field, mdv_cout_uint16); break;
            case MDV_FLD_TYPE_INT32:    mdv_cout_item(row->fields + i, field, mdv_cout_int32);  break;
            case MDV_FLD_TYPE_UINT32:   mdv_cout_item(row->fields + i, field, mdv_cout_uint32); break;
            case MDV_FLD_TYPE_INT64:    mdv_cout_item(row->fields + i, field, mdv_cout_int64);  break;
            case MDV_FLD_TYPE_UINT64:   mdv_cout_item(row->fields + i, field, mdv_cout_uint64); break;
            case MDV_FLD_TYPE_FLOAT:    mdv_cout_item(row->fields + i, field, mdv_cout_float);  break;
            case MDV_FLD_TYPE_DOUBLE:   mdv_cout_item(row->fields + i, field, mdv_cout_double); break;
            default:                    mdv_cout_byte(row->fields + i, field);                  break;
        }
    }

    printf(" |\n");
}


void mdv_cout_table(mdv_table const *table, mdv_enumerator *resultset)
{
    mdv_cout_table_separator(table);
    mdv_cout_table_header(table);
    mdv_cout_table_separator(table);

    while(mdv_enumerator_next(resultset) == MDV_OK)
    {
        mdv_rowset *rowset = mdv_enumerator_current(resultset);

        if (rowset)
        {
            mdv_enumerator *rows_it = mdv_rowset_enumerator(rowset);

            if (rows_it)
            {
                while(mdv_enumerator_next(rows_it) == MDV_OK)
                {
                    mdv_row *row = mdv_enumerator_current(rows_it);
                    mdv_cout_table_row(table, row);
                }

                mdv_enumerator_release(rows_it);
            }

            mdv_rowset_release(rowset);
        }
    }

    mdv_cout_table_separator(table);
}

