#pragma once
#include "../minunit.h"
#include <mdv_list.h>


MU_TEST(platform_list)
{
    mdv_list list = {};

    int n = 0;

    mu_check(mdv_list_push_back(list, n));  n++;
    mu_check(mdv_list_push_back(list, n));  n++;
    mu_check(mdv_list_push_back(list, n));  n++;

    n = 0;

    mdv_list_foreach(list, int, entry)
    {
        mu_check(entry->item == n++);
    }

    mdv_list_remove_next(list, list.next);

    n = 0;

    mdv_list_foreach(list, int, entry)
    {
        mu_check(entry->item == n);
        n += 2;
    }

    mdv_list_clear(list);
}
