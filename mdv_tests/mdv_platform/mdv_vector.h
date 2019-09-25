#pragma once
#include "../minunit.h"
#include <mdv_vector.h>
#include <mdv_alloc.h>


MU_TEST(platform_vector)
{
    mdv_vector *v = mdv_vector_create(2, sizeof(int), &mdv_stallocator);

    mu_check(v);

    int arr[] = { 0, 1, 2 };

    mu_check(mdv_vector_push_back(v, arr + 0));
    mu_check(mdv_vector_push_back(v, arr + 1));
    mu_check(mdv_vector_push_back(v, arr + 2));

    mu_check(mdv_vector_capacity(v) == 4);
    mu_check(mdv_vector_size(v) == 3);

    for(int i = 0; i < 3; ++i)
        mu_check(*(int*)mdv_vector_at(v, i) == i);

    mdv_vector_release(v);
}
