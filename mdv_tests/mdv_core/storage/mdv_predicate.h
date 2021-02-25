#pragma once
#include <minunit.h>
#include <storage/mdv_predicate.h>
#include <mdv_vm.h>


static void mdv_core_storage_predicate_test_0()
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


MU_TEST(core_storage_predicate)
{
    mdv_core_storage_predicate_test_0();
}
