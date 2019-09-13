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

    mdv_mstlink links[] =
    {
        { .src = nodes + 0, .dst = nodes + 1, .weight = 4 },    // 0
        { .src = nodes + 0, .dst = nodes + 7, .weight = 8 },    // 1
        { .src = nodes + 1, .dst = nodes + 7, .weight = 11 },
        { .src = nodes + 1, .dst = nodes + 2, .weight = 8 },
        { .src = nodes + 7, .dst = nodes + 6, .weight = 1 },    // 4
        { .src = nodes + 7, .dst = nodes + 8, .weight = 7 },
        { .src = nodes + 2, .dst = nodes + 8, .weight = 2 },    // 6
        { .src = nodes + 8, .dst = nodes + 6, .weight = 6 },
        { .src = nodes + 2, .dst = nodes + 3, .weight = 7 },    // 8
        { .src = nodes + 6, .dst = nodes + 5, .weight = 2 },    // 9
        { .src = nodes + 2, .dst = nodes + 5, .weight = 4 },    // 10
        { .src = nodes + 3, .dst = nodes + 5, .weight = 14 },
        { .src = nodes + 3, .dst = nodes + 4, .weight = 9 },    // 12
        { .src = nodes + 5, .dst = nodes + 4, .weight = 10 },
    };

    size_t const mst_links[] = { 0, 1, 4, 6, 8, 9, 10, 12 };

    size_t mst_size = mdv_mst_find(nodes, sizeof nodes / sizeof *nodes,
                                   links, sizeof links / sizeof *links);

    mu_check(mst_size == sizeof mst_links / sizeof *mst_links);

    for(size_t i = 0, j = 0; i < sizeof links / sizeof *links; ++i)
        if(links[i].mst)
            mu_check(i == mst_links[j++]);
}
