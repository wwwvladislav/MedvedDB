#pragma once
#include "../minunit.h"
#include <mdv_vector.h>
#include <mdv_alloc.h>


MU_TEST(platform_vector)
{
    mdv_vector(int) v;
    mu_check(mdv_vector_create(v, 2, mdv_stallocator));

    mu_check(mdv_vector_push_back(v, 0));
    mu_check(mdv_vector_push_back(v, 1));
    mu_check(mdv_vector_push_back(v, 2));

    mu_check(v.capacity == 4);
    mu_check(v.size == 3);

    for(int i = 0; i < 3; ++i)
        mu_check(v.data[i] == i);

    mdv_vector_free(v);
}
