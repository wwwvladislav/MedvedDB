#pragma once
#include "mdv_scan_list.h"
#include "mdv_test_utils.h"

#include <minunit.h>
#include <binn.h>
#include <ops/mdv_select.h>
#include <mdv_filesystem.h>
#include <stdio.h>
#include <string.h>


MU_TEST(op_select)
{
    mdv_list table = create_test_rows_list(10, 10);
    mdv_op *scanner = mdv_scan_list(&table);
    mu_check(scanner);

    mdv_predicate *predicate = mdv_predicate_parse("");
    mu_check(predicate);

    mdv_op * select = mdv_select(scanner, predicate);
    mu_check(select);

    mdv_predicate_release(predicate);
    predicate = 0;
    mdv_op_release(scanner);
    scanner = 0;

    mdv_kvdata kvdata;

    for(size_t i = 0; i < 10; ++i)
    {
        mu_check(mdv_op_next(select, &kvdata) == MDV_OK);

        mu_check(kvdata.key.size == sizeof(size_t));
        mu_check(*(size_t*)kvdata.key.ptr == i);

        binn row;
        mu_check(binn_load(kvdata.value.ptr, &row));

        binn_iter iter = {};
        binn item = {};

        uint32_t n = 0;

        binn_list_foreach(&row, item)
        {
            int value = 0;
            mu_check(binn_get_int32(&item, &value));
            mu_check((uint32_t)value == n++);
        }
    }

    mu_check(mdv_op_next(select, &kvdata) == MDV_FALSE);
    mdv_op_reset(select);
    mu_check(mdv_op_next(select, &kvdata) == MDV_OK);

    mdv_list_clear(&table);
    mdv_op_release(select);
}

