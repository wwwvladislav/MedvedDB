#pragma once
#include "../minunit.h"
#include <mdv_mst.h>


MU_TEST(platform_mst)
{
    mdv_mstnode const nodes[] =
    {
        { .data = (void *)0 },
        { .data = (void *)1 },
        { .data = (void *)2 },
        { .data = (void *)3 },
        { .data = (void *)4 },
        { .data = (void *)5 },
        { .data = (void *)6 },
        { .data = (void *)7 },
        { .data = (void *)8 },
    };

    mdv_mstlink const links[] =
    {
        { .src = nodes + 0, .dst = nodes + 1, .weight = 4 },
        { .src = nodes + 0, .dst = nodes + 7, .weight = 8 },
        { .src = nodes + 1, .dst = nodes + 7, .weight = 11 },
        { .src = nodes + 1, .dst = nodes + 2, .weight = 8 },
        { .src = nodes + 7, .dst = nodes + 6, .weight = 1 },
        { .src = nodes + 7, .dst = nodes + 8, .weight = 7 },
        { .src = nodes + 2, .dst = nodes + 8, .weight = 2 },
        { .src = nodes + 8, .dst = nodes + 6, .weight = 6 },
        { .src = nodes + 2, .dst = nodes + 3, .weight = 7 },
        { .src = nodes + 6, .dst = nodes + 5, .weight = 2 },
        { .src = nodes + 2, .dst = nodes + 5, .weight = 4 },
        { .src = nodes + 3, .dst = nodes + 5, .weight = 14 },
        { .src = nodes + 3, .dst = nodes + 4, .weight = 9 },
        { .src = nodes + 5, .dst = nodes + 4, .weight = 10 },
    };

    mdv_mst *mst = mdv_mst_find(nodes, sizeof nodes / sizeof *nodes,
                                links, sizeof links / sizeof *links);
/*
    mdv_list list = {};

    int n = 0;

    mu_check(mdv_list_push_back(list, n));  n++;
    mu_check(mdv_list_push_back(list, n));  n++;
    mu_check(mdv_list_push_back(list, n));  n++;

    n = 0;

    mdv_list_foreach(list, int, entry)
    {
        mu_check(*entry == n++);
    }

    mdv_list_remove(list, list.next->next);

    n = 0;

    mdv_list_foreach(list, int, entry)
    {
        mu_check(*entry == n);
        n += 2;
    }

    mdv_list_pop_back(list);

    n = 0;

    mdv_list_foreach(list, int, entry)
    {
        mu_check(*entry == n);
        n++;
    }

    mu_check(n == 1);

    mdv_list_clear(list);
*/
}
