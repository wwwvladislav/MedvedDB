#pragma once
#include "../minunit.h"
#include <mdv_serialization.h>
#include <mdv_alloc.h>
#include <mdv_uuid.h>


static void test_table_serialization()
{
    mdv_field const fields[] =
    {
        { MDV_FLD_TYPE_CHAR,   0, mdv_str_static("string") },   // char *
        { MDV_FLD_TYPE_BOOL,   1, mdv_str_static("bool") },     // bool
        { MDV_FLD_TYPE_UINT64, 2, mdv_str_static("u64pair") }   // uint64[2]
    };

    mdv_table_desc desc =
    {
        .name = mdv_str_static("my_table"),
        .size = 3,
        .fields = fields
    };

    mdv_uuid uuid = { .u64 = { 1, 2 } };

    mdv_table *table = mdv_table_create(&uuid, &desc);
    mu_check(table);

    binn obj;
    mu_check(mdv_binn_table(table, &obj));
    mdv_table *deserialized_table = mdv_unbinn_table(&obj);

    mu_check(mdv_uuid_cmp(&uuid, mdv_table_uuid(table)) == 0);
    mu_check(mdv_uuid_cmp(&uuid, mdv_table_uuid(deserialized_table)) == 0);

    mdv_table_desc const *deserialized_desc = mdv_table_description(deserialized_table);

    mu_check(deserialized_desc->size == desc.size);
    mu_check(deserialized_desc->name.size == desc.name.size);
    mu_check(memcmp(deserialized_desc->name.ptr, desc.name.ptr, desc.name.size) == 0);

    for(uint32_t i = 0; i < desc.size; ++i)
    {
        mu_check(deserialized_desc->fields[i].type == desc.fields[i].type);
        mu_check(deserialized_desc->fields[i].limit == desc.fields[i].limit);
        mu_check(deserialized_desc->fields[i].name.size == desc.fields[i].name.size);
        mu_check(memcmp(deserialized_desc->fields[i].name.ptr, desc.fields[i].name.ptr, desc.fields[i].name.size) == 0);
    }

    binn_free(&obj);
    mdv_table_release(deserialized_table);
    mdv_table_release(table);
}


static void test_row_serialization()
{
    int arr[] =  { 41, 42 };

    mdv_row(3) row =
    {
        .size = 3,
        .fields =
        {
            { 1, "1" },
            { 2, "12" },
            { sizeof(arr), arr }
        }
    };

    mdv_field fields[] =
    {
        { MDV_FLD_TYPE_CHAR,  1, mdv_str_static("col1") },
        { MDV_FLD_TYPE_CHAR,  2, mdv_str_static("col2") },
        { MDV_FLD_TYPE_INT32, 3, mdv_str_static("col3") }
    };

    binn obj;

    mu_check(mdv_binn_row(fields, (mdv_row_base const *)&row, &obj));

    mdv_row_base *deserialized_row = mdv_unbinn_row(&obj, fields);

    mu_check(deserialized_row->size == row.size);

    for(uint32_t i = 0; i < row.size; ++i)
    {
        mu_check(deserialized_row->fields[i].size == row.fields[i].size);
        mu_check(memcmp(deserialized_row->fields[i].ptr, row.fields[i].ptr, row.fields[i].size) == 0);
    }

    binn_free(&obj);
    mdv_free(deserialized_row, "row");
}


MU_TEST(types_serialization)
{
    test_table_serialization();
    test_row_serialization();
}
