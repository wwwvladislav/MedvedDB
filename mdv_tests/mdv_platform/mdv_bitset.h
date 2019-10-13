#pragma once
#include "../minunit.h"
#include <mdv_bitset.h>


MU_TEST(platform_bitset)
{
    mdv_bitset *bitset = mdv_bitset_create(33, &mdv_stallocator);

    mu_check(mdv_bitset_set(bitset, 0));
    mu_check(mdv_bitset_set(bitset, 32));
    mu_check(mdv_bitset_set(bitset, 15));
    mu_check(!mdv_bitset_set(bitset, 15));

    mu_check(mdv_bitset_test(bitset, 0));
    mu_check(mdv_bitset_test(bitset, 32));
    mu_check(mdv_bitset_test(bitset, 15));

    mdv_bitset_reset(bitset, 15);
    mu_check(!mdv_bitset_test(bitset, 15));

    mdv_bitset_free(bitset);
}
