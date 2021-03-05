#pragma once
#include "mdv_scan_list.h"

#include <minunit.h>
#include <binn.h>
#include <ops/mdv_project.h>
#include <mdv_filesystem.h>
#include <stdio.h>
#include <string.h>


static binn * create_row(size_t size, uint32_t init)
{
    binn *row = binn_list();
    uint32_t n = init;
    for (size_t i = 0; i < size; ++i)
        binn_list_add_uint32(row, n++);
    return row;
}


static mdv_list create_rows_list(size_t rows, size_t colums)
{
    mdv_list list = {};

    for (size_t i = 0; i < rows; ++i)
    {
        binn *row = create_row(colums, 0);
        mdv_list_push_back_data(&list, binn_ptr(row), binn_size(row));
        binn_free(row);
    }

    return list;
}


MU_TEST(op_project)
{
    mdv_list table = create_rows_list(10, 10);
    mdv_op *scanner = mdv_scan_list(&table);
    mu_check(scanner);

    const uint32_t from = 2, to = 7;

    mdv_op * project = mdv_project_range(scanner, from, to);
    mu_check(project);

    mdv_op_release(scanner);
    scanner = 0;

    mdv_kvdata kvdata;

    for(size_t i = 0; i < 10; ++i)
    {
        mu_check(mdv_op_next(project, &kvdata) == MDV_OK);

        mu_check(kvdata.key.size == sizeof(size_t));
        mu_check(*(size_t*)kvdata.key.ptr == i);

        binn row;
        mu_check(binn_load(kvdata.value.ptr, &row));

        binn_iter iter = {};
        binn item = {};

        uint32_t n = from;

        binn_list_foreach(&row, item)
        {
            int value = 0;
            mu_check(binn_get_int32(&item, &value));
            mu_check((uint32_t)value == n++);
        }
    }

    mu_check(mdv_op_next(project, &kvdata) == MDV_FALSE);
    mdv_op_reset(project);
    mu_check(mdv_op_next(project, &kvdata) == MDV_OK);

    mdv_list_clear(&table);
    mdv_op_release(project);
}
