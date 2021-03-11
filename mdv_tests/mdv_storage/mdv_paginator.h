#pragma once
#include <minunit.h>
#include <mdv_paginator.h>


/*
static void mdv_storage_predicate_test_0()
{
    mdv_predicate *predicate = mdv_predicate_parse("");
    mu_check(predicate);

    mdv_stack(uint8_t, 64) stack;
    mdv_stack_clear(stack);

    mu_check(mdv_vm_run((mdv_stack_base*)&stack,
                        mdv_predicate_fns(predicate),
                        mdv_predicate_fns_count(predicate),
                        mdv_predicate_expr(predicate)) == MDV_OK);

    bool res = false;
    mu_check(mdv_vm_result_as_bool((mdv_stack_base*)&stack, &res) == MDV_OK);
    mu_check(res);

    mdv_predicate_release(predicate);
}
*/


MU_TEST(storage_paginator)
{
    mdv_paginator *paginator = mdv_paginator_open(5, 10, "./test_pages");
    mu_check(paginator);

    mu_check(mdv_paginator_release(paginator) == 0);
    mdv_rmdir("./test_pages");
}
