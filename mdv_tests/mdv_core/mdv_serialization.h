#pragma once
#include "../minunit.h"
#include <storage/mdv_serialization.h>
#include <mdv_alloc.h>


static void test_table_serialization()
{
    mdv_table(3) tbl =
    {
        .uuid = mdv_uuid_generate(),
        .name = mdv_str_static("my_table"),
        .size = 3,
        .fields =
        {
            { MDV_FLD_TYPE_CHAR,   0, mdv_str_static("string") },   // char *
            { MDV_FLD_TYPE_BOOL,   1, mdv_str_static("bool") },     // bool
            { MDV_FLD_TYPE_UINT64, 2, mdv_str_static("u64pair") }   // uint64[2]
        }
    };

    binn * obj = mdv_binn_table((mdv_table_base const *)&tbl);
    mdv_table_base * deserialized_tbl = mdv_unbinn_table(obj);

    mu_check(mdv_uuid_cmp(&deserialized_tbl->uuid, &tbl.uuid) == 0);
    mu_check(deserialized_tbl->size == tbl.size);
    mu_check(deserialized_tbl->name.size == tbl.name.size);
    mu_check(memcmp(deserialized_tbl->name.ptr, tbl.name.ptr, tbl.name.size) == 0);

    for(uint32_t i = 0; i < tbl.size; ++i)
    {
        mu_check(deserialized_tbl->fields[i].type == tbl.fields[i].type);
        mu_check(deserialized_tbl->fields[i].limit == tbl.fields[i].limit);
        mu_check(deserialized_tbl->fields[i].name.size == tbl.fields[i].name.size);
        mu_check(memcmp(deserialized_tbl->fields[i].name.ptr, tbl.fields[i].name.ptr, tbl.fields[i].name.size) == 0);
    }

    binn_free(obj);
    mdv_free(deserialized_tbl);
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

    binn *obj = mdv_binn_row(fields, (mdv_row_base const *)&row);
    mdv_row_base *deserialized_row = mdv_unbinn_row(obj, fields);

    mu_check(deserialized_row->size == row.size);

    for(uint32_t i = 0; i < row.size; ++i)
    {
        mu_check(deserialized_row->fields[i].size == row.fields[i].size);
        mu_check(memcmp(deserialized_row->fields[i].ptr, row.fields[i].ptr, row.fields[i].size) == 0);
    }

    binn_free(obj);
    mdv_free(deserialized_row);
}


MU_TEST(core_serialization)
{
    test_table_serialization();
    test_row_serialization();
}
