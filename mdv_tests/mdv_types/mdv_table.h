#pragma once
#include "../minunit.h"
#include <mdv_table.h>
#include <mdv_alloc.h>


MU_TEST(types_table)
{
    mdv_field const fields[] =
    {
        { MDV_FLD_TYPE_CHAR, 0, mdv_str_static("Col1") },   // char *
        { MDV_FLD_TYPE_CHAR, 0, mdv_str_static("Col2") },   // char *
        { MDV_FLD_TYPE_CHAR, 0, mdv_str_static("Col3") }    // char *
    };

    mdv_table_desc desc =
    {
        .name = mdv_str_static("MyTable"),
        .size = 3,
        .fields = fields
    };

    mdv_uuid const uuid = { .a = 42 };

    mdv_table *table = mdv_table_create(&uuid, &desc);

    mdv_bitset *mask = mdv_bitset_create(desc.size, &mdv_default_allocator);
    mdv_bitset_set(mask, 0);
    mdv_bitset_set(mask, 2);

    mdv_table *slice = mdv_table_slice(table, mask);

    mu_check(mdv_uuid_cmp(mdv_table_uuid(table), mdv_table_uuid(slice)) == 0);
    mu_check(strcmp(mdv_table_description(table)->name.ptr,
                    mdv_table_description(slice)->name.ptr) == 0);
    mu_check(mdv_table_description(slice)->size == 2);

    mu_check(mdv_table_description(table)->fields[0].type ==
             mdv_table_description(slice)->fields[0].type);
    mu_check(mdv_table_description(table)->fields[0].limit ==
             mdv_table_description(slice)->fields[0].limit);
    mu_check(strcmp(mdv_table_description(table)->fields[0].name.ptr,
                    mdv_table_description(slice)->fields[0].name.ptr) == 0);

    mu_check(mdv_table_description(table)->fields[2].type ==
             mdv_table_description(slice)->fields[1].type);
    mu_check(mdv_table_description(table)->fields[2].limit ==
             mdv_table_description(slice)->fields[1].limit);
    mu_check(strcmp(mdv_table_description(table)->fields[2].name.ptr,
                    mdv_table_description(slice)->fields[1].name.ptr) == 0);

    mu_check(mdv_bitset_release(mask) == 0);
    mu_check(mdv_table_release(table) == 0);
    mu_check(mdv_table_release(slice) == 0);
}
