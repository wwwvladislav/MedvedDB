#pragma once
#include "../minunit.h"
#include <mdv_string.h>

MU_TEST(platform_string)
{
    {
        mdv_stack(char, 256) stack;
        mdv_stack_clear(stack);

        mdv_string str1 = mdv_str_pdup(stack, "456");
        mdv_string str2 = mdv_str_pdup(stack, "123");
        mdv_string str3 = mdv_str_pcat(stack, str1, str2);
        mu_check(mdv_str_empty(str3));

        str3 = mdv_str_pcat(stack, str2, str1);
        mu_check(!mdv_str_empty(str3));
        mu_check(strcmp(str3.ptr, "123456") == 0);
    }

    {
        mdv_stack(char, 256) stack;
        mdv_stack_clear(stack);

        mdv_string str1 = mdv_str_pdup(stack, "789");
        mdv_string str2 = mdv_str_pdup(stack, "456");
        mdv_string str3 = mdv_str_pdup(stack, "123");
        mdv_string str4 = mdv_str_pcat(stack, str3, str2, str1);

        mu_check(!mdv_str_empty(str4));
        mu_check(strcmp(str4.ptr, "123456789") == 0);
    }
}
