#pragma once
#include "../minunit.h"
#include <mdv_serialization.h>
#include <mdv_alloc.h>
#include <mdv_uuid.h>


static void test_uuid_serialization()
{
    mdv_uuid uuid = { .u64 = { 0x1122334455667788, 0x99AABBCCDDEEFF00 } };
    mdv_uuid deserialized_uuid;

    binn obj;

    mu_check(mdv_binn_uuid(&uuid, &obj));
    mu_check(mdv_unbinn_uuid(&obj, &deserialized_uuid));
    mu_check(mdv_uuid_cmp(&deserialized_uuid, &uuid) == 0);

    binn_free(&obj);
}


static void test_table_serialization()
{
    mdv_field const fields[] =
    {
        { MDV_FLD_TYPE_CHAR,   0, "string" },   // char *
        { MDV_FLD_TYPE_BOOL,   1, "bool" },     // bool
        { MDV_FLD_TYPE_UINT64, 2, "u64pair" }   // uint64[2]
    };

    mdv_table_desc desc =
    {
        .name = "my_table",
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
    mu_check(strcmp(deserialized_desc->name, desc.name) == 0);

    for(uint32_t i = 0; i < desc.size; ++i)
    {
        mu_check(deserialized_desc->fields[i].type == desc.fields[i].type);
        mu_check(deserialized_desc->fields[i].limit == desc.fields[i].limit);
        mu_check(strcmp(deserialized_desc->fields[i].name, desc.fields[i].name) == 0);
    }

    binn_free(&obj);
    mdv_table_release(deserialized_table);
    mdv_table_release(table);
}


static void test_rowset_serialization()
{
    mdv_field fields[] =
    {
        { MDV_FLD_TYPE_CHAR,  1, "col1" },
        { MDV_FLD_TYPE_CHAR,  2, "col2" },
        { MDV_FLD_TYPE_INT32, 3, "col3" }
    };

    mdv_table_desc desc =
    {
        .name = "MyTable",
        .size = 3,
        .fields = fields
    };

    mdv_uuid const uuid = {};

    mdv_table *table = mdv_table_create(&uuid, &desc);
    mdv_rowset *rowset = mdv_rowset_create(table);

    int arr1[] =  { 41, 42 };
    int arr2[] =  { 43, 44 };

    mdv_data const row0[] = { { 1, "1" }, { 2, "12" }, { sizeof(arr1), arr1 } };
    mdv_data const row1[] = { { 1, "2" }, { 2, "34" }, { sizeof(arr2), arr2 } };
    mdv_data const *rows[] = { row0, row1 };

    mu_check(mdv_rowset_append(rowset, rows, sizeof rows / sizeof *rows) == sizeof rows / sizeof *rows);

    binn serialized_rowset;

    mu_check(mdv_binn_rowset(rowset, &serialized_rowset));

    mdv_rowset *deserialized_rowset = mdv_unbinn_rowset(&serialized_rowset, table);

    mdv_enumerator *enumerator1 = mdv_rowset_enumerator(rowset);
    mdv_enumerator *enumerator2 = mdv_rowset_enumerator(deserialized_rowset);

    size_t rows_count = 0;

    uint32_t const columns = desc.size;

    while(mdv_enumerator_next(enumerator1) == MDV_OK
          && mdv_enumerator_next(enumerator2) == MDV_OK)
    {
        mdv_row *row1 = mdv_enumerator_current(enumerator1);
        mdv_row *row2 = mdv_enumerator_current(enumerator2);

        for(uint32_t i = 0; i < columns; ++i)
        {
            mdv_data const *entry1 = &row1->fields[i];
            mdv_data const *entry2 = &row2->fields[i];

            mu_check(entry1->size == entry2->size);
            mu_check(memcmp(entry1->ptr, entry2->ptr, entry2->size) == 0);
        }

        ++rows_count;
    }

    mu_check(rows_count == sizeof rows / sizeof *rows);

    binn_free(&serialized_rowset);

    mu_check(mdv_enumerator_release(enumerator1) == 0);
    mu_check(mdv_enumerator_release(enumerator2) == 0);
    mu_check(mdv_rowset_release(rowset) == 0);
    mu_check(mdv_rowset_release(deserialized_rowset) == 0);
    mu_check(mdv_table_release(table) == 0);
}


static void test_row_slice_serialization()
{
    mdv_field fields[] =
    {
        { MDV_FLD_TYPE_CHAR,  1, "col1" },
        { MDV_FLD_TYPE_CHAR,  2, "col2" },
        { MDV_FLD_TYPE_INT32, 3, "col3" }
    };

    mdv_table_desc desc =
    {
        .name = "MyTable",
        .size = 3,
        .fields = fields
    };

    int arr[] =  { 41, 42 };

    mdv_data const row[] = { { 1, "1" }, { 2, "12" }, { sizeof(arr), arr } };

    binn serialized_row;

    mu_check(mdv_binn_row((mdv_row const *)row, &desc, &serialized_row));

    mdv_bitset *mask = mdv_bitset_create(desc.size, &mdv_default_allocator);

    mu_check(mdv_bitset_set(mask, 0));
    mu_check(mdv_bitset_set(mask, 2));

    mdv_rowlist_entry *rowlist_entry = mdv_unbinn_row_slice(&serialized_row, &desc, mask);
    mu_check(rowlist_entry);

    mu_check(rowlist_entry->data.fields[0].size == row[0].size);
    mu_check(rowlist_entry->data.fields[1].size == row[2].size);

    mu_check(memcmp(rowlist_entry->data.fields[0].ptr, row[0].ptr, row[0].size) == 0);
    mu_check(memcmp(rowlist_entry->data.fields[1].ptr, row[2].ptr, row[2].size) == 0);

    mdv_free(rowlist_entry, "rowlist_entry");
    binn_free(&serialized_row);
    mdv_bitset_release(mask);
}


MU_TEST(types_serialization)
{
    test_uuid_serialization();
    test_table_serialization();
    test_rowset_serialization();
    test_row_slice_serialization();
}
