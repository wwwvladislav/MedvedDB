#pragma once
#include "../minunit.h"
#include <mdv_topology.h>
#include <stdint.h>


static void mdv_topology_links_check(mdv_topology const *t, mdv_topolink const *links, size_t links_count)
{
    mu_check(t->links_count == links_count);

    for(size_t i = 0; i < links_count; ++i)
    {
        mu_check(mdv_link_cmp(t->links + i, links + i) == 0);
        mu_check(t->links[i].weight == links[i].weight);
    }
}


static void mdv_topology_test_1()
{
    /*
        Topology 'a':
            1 - 2
             \
              3
        Topology 'b':
            1 - 2
             \
              4
    */

    mdv_toponode nodes[] =
    {
        { .uuid = { .a = 1 }, .addr = "1" },
        { .uuid = { .a = 2 }, .addr = "2" },
        { .uuid = { .a = 3 }, .addr = "3" },
        { .uuid = { .a = 4 }, .addr = "4" },
    };

    mdv_topolink a_links[] =
    {
        { .node = { nodes + 0, nodes + 1 }, .weight = 1 },
        { .node = { nodes + 0, nodes + 2 }, .weight = 1 },
    };

    mdv_topolink b_links[] =
    {
        { .node = { nodes + 0, nodes + 1 }, .weight = 1 },
        { .node = { nodes + 0, nodes + 3 }, .weight = 1 },
    };

    mdv_topolink ab_links[] =
    {
        { .node = { nodes + 0, nodes + 2 }, .weight = 1 },
    };

    mdv_topolink ba_links[] =
    {
        { .node = { nodes + 0, nodes + 3 }, .weight = 1 },
    };

    mdv_topology const a =
    {
        .nodes_count = sizeof nodes / sizeof *nodes,
        .links_count = sizeof a_links / sizeof *a_links,
        .nodes = nodes,
        .links = a_links
    };

    mdv_topology const b =
    {
        .nodes_count = sizeof nodes / sizeof *nodes,
        .links_count = sizeof b_links / sizeof *b_links,
        .nodes = nodes,
        .links = b_links
    };

    mdv_topology_delta *delta = mdv_topology_diff(&a, &b);

    mdv_topology_links_check(delta->ab, ab_links, sizeof ab_links / sizeof *ab_links);
    mdv_topology_links_check(delta->ba, ba_links, sizeof ba_links / sizeof *ba_links);

    mdv_topology_delta_free(delta);
}


static void mdv_topology_test_2()
{
    /*
        Topology 'a':
            1 - 2
             \
              3
        Topology 'b':
            1 - 2
    */

    mdv_toponode nodes[] =
    {
        { .uuid = { .a = 1 }, .addr = "1" },
        { .uuid = { .a = 2 }, .addr = "2" },
        { .uuid = { .a = 3 }, .addr = "3" },
    };

    mdv_topolink a_links[] =
    {
        { .node = { nodes + 0, nodes + 1 }, .weight = 1 },
        { .node = { nodes + 0, nodes + 2 }, .weight = 1 },
    };

    mdv_topolink b_links[] =
    {
        { .node = { nodes + 0, nodes + 1 }, .weight = 1 },
    };

    mdv_topolink ab_links[] =
    {
        { .node = { nodes + 0, nodes + 2 }, .weight = 1 },
    };

    mdv_topology const a =
    {
        .nodes_count = sizeof nodes / sizeof *nodes,
        .links_count = sizeof a_links / sizeof *a_links,
        .nodes = nodes,
        .links = a_links
    };

    mdv_topology const b =
    {
        .nodes_count = sizeof nodes / sizeof *nodes,
        .links_count = sizeof b_links / sizeof *b_links,
        .nodes = nodes,
        .links = b_links
    };

    mdv_topology_delta *delta = mdv_topology_diff(&a, &b);

    mdv_topology_links_check(delta->ab, ab_links, sizeof ab_links / sizeof *ab_links);

    mu_check(delta->ba->nodes_count == 0);
    mu_check(delta->ba->links_count == 0);
    mu_check(delta->ba->nodes == 0);
    mu_check(delta->ba->links == 0);

    mdv_topology_delta_free(delta);
}


static void mdv_topology_test_3()
{
    /*
        Topology 'a':
            1 - 2
        Topology 'b':
            1 - 2
    */

    mdv_toponode nodes[] =
    {
        { .uuid = { .a = 1 }, .addr = "1" },
        { .uuid = { .a = 2 }, .addr = "2" },
    };

    mdv_topolink links[] =
    {
        { .node = { nodes + 0, nodes + 1 }, .weight = 1 },
    };

    mdv_topology const a =
    {
        .nodes_count = sizeof nodes / sizeof *nodes,
        .links_count = sizeof links / sizeof *links,
        .nodes = nodes,
        .links = links
    };

    mdv_topology_delta *delta = mdv_topology_diff(&a, &a);

    mu_check(delta->ab->nodes_count == 0);
    mu_check(delta->ab->links_count == 0);
    mu_check(delta->ab->nodes == 0);
    mu_check(delta->ab->links == 0);

    mu_check(delta->ba->nodes_count == 0);
    mu_check(delta->ba->links_count == 0);
    mu_check(delta->ba->nodes == 0);
    mu_check(delta->ba->links == 0);

    mdv_topology_delta_free(delta);
}


MU_TEST(platform_topology)
{
    mdv_topology_test_1();
    mdv_topology_test_2();
    mdv_topology_test_3();
}

