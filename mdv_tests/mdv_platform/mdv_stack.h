#pragma once
#include <minunit.h>
#include <mdv_stack.h>


MU_TEST(platform_stack)
{
    mdv_stack(char, 5) stack;
    mdv_stack_clear(stack);

    char arr[2] = { '0', '1' };
    mu_check(*mdv_stack_push(stack, arr, 2) == '0');

    mu_check(*mdv_stack_push(stack, "23", 2) == '2');

    mu_check(*mdv_stack_push(stack, (char)'4') == '4');

    mu_check(*mdv_stack_top(stack) == '4');

    char r = '4';
    mdv_stack_foreach(stack, char, ch)
    {
        mu_check(*ch == r--);
    }

    mu_check(mdv_stack_size(stack) == mdv_stack_capacity(stack));

    mu_check(*(char const*)mdv_stack_pop(stack) == '4');

    mu_check(*(char const*)mdv_stack_pop(stack, 2) == '2');

    mu_check(*(char const*)mdv_stack_pop(stack, 1) == '1');

    mu_check(*(char const*)mdv_stack_pop(stack, 1) == '0');

    mu_check(mdv_stack_empty(stack));
}
