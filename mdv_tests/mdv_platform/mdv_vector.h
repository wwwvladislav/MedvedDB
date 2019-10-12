#pragma once
#include "../minunit.h"
#include <mdv_vector.h>
#include <mdv_alloc.h>


static bool mdv_int_equ(void const *a, void const *b)
{
    return *(int const *)a == *(int const *)b;
}


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

    int const *f = mdv_vector_find(v, arr + 1, mdv_int_equ);
    mu_check(*f == arr[1]);
    mdv_vector_erase(v, f);
    mu_check(mdv_vector_capacity(v) == 4);
    mu_check(mdv_vector_size(v) == 2);
    mu_check(*(int*)mdv_vector_at(v, 0) == 0);
    mu_check(*(int*)mdv_vector_at(v, 1) == 2);


    f = mdv_vector_find(v, arr + 2, mdv_int_equ);
    mdv_vector_erase(v, f);
    mu_check(mdv_vector_capacity(v) == 4);
    mu_check(mdv_vector_size(v) == 1);
    mu_check(*(int*)mdv_vector_at(v, 0) == 0);

    int arr1[] = { 1, 2, 3, 4, 5 };
    mu_check(mdv_vector_append(v, arr1, sizeof arr1 / sizeof *arr1));

    for(int i = 0; i < 6; ++i)
        mu_check(*(int*)mdv_vector_at(v, i) == i);

    mdv_vector_release(v);
}
