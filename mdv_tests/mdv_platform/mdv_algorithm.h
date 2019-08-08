#pragma once
#include "../minunit.h"
#include <mdv_algorithm.h>


static int mdv_test_u32_cmp(void const *a, void const *b)
{
    return *(uint32_t*)a - *(uint32_t*)b;
}


#define mdv_test_union_and_diff(set_a, set_b, res_union, res_diff)      \
    {                                                                   \
        uint32_t sets_union[sizeof res_union / sizeof *res_union] = {}; \
        uint32_t sets_diff[sizeof res_diff / sizeof *res_diff] = {};    \
        uint32_t union_size, diff_size;                                 \
                                                                        \
        mdv_diff_u32(set_a, sizeof set_a / sizeof *set_a,               \
                     set_b, sizeof set_b / sizeof *set_b,               \
                     sets_diff, &diff_size);                            \
        mdv_union_u32(set_a, sizeof set_a / sizeof *set_a,              \
                      set_b, sizeof set_b / sizeof *set_b,              \
                      sets_union, &union_size);                         \
                                                                        \
        mu_check(union_size == sizeof res_union / sizeof *res_union);   \
        mu_check(diff_size == sizeof res_diff / sizeof *res_diff);      \
                                                                        \
        for(uint32_t i = 0; i < union_size; ++i)                        \
            mu_check(sets_union[i] == res_union[i]);                    \
                                                                        \
        for(uint32_t i = 0; i < diff_size; ++i)                         \
            mu_check(sets_diff[i] == res_diff[i]);                      \
                                                                        \
        uint32_t excl_size = mdv_exclude(set_a,                         \
                                         sizeof(uint32_t),              \
                                         sizeof set_a / sizeof *set_a,  \
                                         set_b,                         \
                                         sizeof(uint32_t),              \
                                         sizeof set_b / sizeof *set_b,  \
                                         &mdv_test_u32_cmp);            \
                                                                        \
        mu_check(excl_size == diff_size);                               \
                                                                        \
        for(uint32_t i = 0; i < diff_size; ++i)                         \
            mu_check(set_a[i] == res_diff[i]);                          \
    }


MU_TEST(platform_algorithm)
{
    {   // Test 1
        uint32_t set_a[]        = { 1, 2, 3, 4, 5 };
        uint32_t set_b[]        = {       3, 4, 5, 6, 7 };
        uint32_t res_union[]    = { 1, 2, 3, 4, 5, 6, 7 };
        uint32_t res_diff[]     = { 1, 2 };

        mdv_test_union_and_diff(set_a, set_b, res_union, res_diff);
    }

    {   // Test 2
        uint32_t set_a[]        = { 1, 2, 3, 4, 5, 5, 5 };
        uint32_t set_b[]        = {       3, 4, 5, 6, 7 };
        uint32_t res_union[]    = { 1, 2, 3, 4, 5, 5, 5, 6, 7 };
        uint32_t res_diff[]     = { 1, 2, 5, 5 };

        mdv_test_union_and_diff(set_a, set_b, res_union, res_diff);
    }

    {   // Test 3
        uint32_t set_a[]        = { 1, 2, 3, 4, 5 };
        uint32_t set_b[]        = { 1, 2, 3, 4, 5 };
        uint32_t res_union[]    = { 1, 2, 3, 4, 5 };
        uint32_t res_diff[0];

        mdv_test_union_and_diff(set_a, set_b, res_union, res_diff);
    }

    {   // Test 4
        uint32_t set_a[]        = { 2, 3 };
        uint32_t set_b[]        = { 1 };
        uint32_t res_union[]    = { 1, 2, 3 };
        uint32_t res_diff[]     = { 2, 3 };

        mdv_test_union_and_diff(set_a, set_b, res_union, res_diff);
    }
}


#undef mdv_test_union_and_diff
