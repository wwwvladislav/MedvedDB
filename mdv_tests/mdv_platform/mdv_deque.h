#pragma once
#include "../minunit.h"
#include <mdv_deque.h>
#include <stdint.h>


MU_TEST(platform_deque)
{
    mdv_deque *deque = mdv_deque_create(sizeof(uint32_t));
    mu_check(deque);

    for(uint32_t i = 0; i < 20000; ++i)
        mdv_deque_push_back(deque, &i, sizeof i);

    for(uint32_t i = 0; i < 20000; ++i)
    {
        uint32_t v = 0;
        mdv_deque_pop_back(deque, &v);
        if (i > 19996 || (i % 100) == 0)
            mu_check(v == 20000 - i - 1);
    }

    mdv_deque_free(deque);
}
