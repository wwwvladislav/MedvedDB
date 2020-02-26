#pragma once
#include "../minunit.h"
#include <mdv_rowset.h>
#include <mdv_alloc.h>
#include <mdv_uuid.h>


MU_TEST(types_rowset)
{
    mdv_field const fields[] =
    {
        { MDV_FLD_TYPE_CHAR, 0, "Col1" },       // char *
        { MDV_FLD_TYPE_CHAR, 0, "Col2" },       // char *
        { MDV_FLD_TYPE_CHAR, 0, "Col3" }        // char *
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

    mdv_data const row0[] = { { 2, "00" }, { 2, "01" }, { 2, "02" } };
    mdv_data const row1[] = { { 2, "10" }, { 2, "11" }, { 2, "12" } };
    mdv_data const *rows[] = { row0, row1 };

    mu_check(mdv_rowset_append(rowset, rows, sizeof rows / sizeof *rows) == sizeof rows / sizeof *rows);

    mdv_enumerator *enumerator = mdv_rowset_enumerator(rowset);

    size_t rows_count = 0;

    uint32_t const columns = desc.size;

    while(mdv_enumerator_next(enumerator) == MDV_OK)
    {
        mdv_row *row = mdv_enumerator_current(enumerator);

        for(uint32_t i = 0; i < columns; ++i)
        {
            mdv_data const *orig = &rows[rows_count][i];
            mdv_data const *entry = &row->fields[i];

            mu_check(orig->size == entry->size);
            mu_check(memcmp(orig->ptr, entry->ptr, entry->size) == 0);
        }

        ++rows_count;
    }

    mu_check(rows_count == sizeof rows / sizeof *rows);
    mu_check(mdv_enumerator_release(enumerator) == 0);
    mu_check(mdv_rowset_release(rowset) == 0);
    mu_check(mdv_table_release(table) == 0);
}
