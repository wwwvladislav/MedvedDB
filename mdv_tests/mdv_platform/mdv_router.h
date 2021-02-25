#pragma once
#include <minunit.h>
#include <mdv_router.h>


static mdv_topology * mdv_test_router_topology_create(mdv_toponode *nodes, size_t nsize,
                                                      mdv_topolink *links, size_t lsize)
{
    mdv_vector *toponodes = mdv_vector_create(nsize, sizeof(mdv_toponode), &mdv_default_allocator);
    mdv_vector_append(toponodes, nodes, nsize);

    mdv_vector *topolinks = mdv_vector_create(lsize, sizeof(mdv_topolink), &mdv_default_allocator);
    mdv_vector_append(topolinks, links, lsize);

    mdv_topology *topology = mdv_topology_create(toponodes, topolinks, &mdv_empty_vector);

    mdv_vector_release(toponodes);
    mdv_vector_release(topolinks);

    return topology;
}


void mdv_platform_router_test_1()
{
    /*
        Topology:
              1   4
             / \ /
            0   3
             \ / \
              2   5
    */

    mdv_toponode nodes[] =
    {
        { .id = 0, .uuid = { .a = 0 }, .addr = "0" },
        { .id = 1, .uuid = { .a = 1 }, .addr = "1" },
        { .id = 2, .uuid = { .a = 2 }, .addr = "2" },
        { .id = 3, .uuid = { .a = 3 }, .addr = "3" },
        { .id = 4, .uuid = { .a = 4 }, .addr = "4" },
        { .id = 5, .uuid = { .a = 5 }, .addr = "5" },
    };

    mdv_topolink links[] =
    {
        { .node = { 0, 1 }, .weight = 1 },
        { .node = { 0, 2 }, .weight = 1 },
        { .node = { 1, 3 }, .weight = 1 },
        { .node = { 2, 3 }, .weight = 1 },
        { .node = { 3, 4 }, .weight = 1 },
        { .node = { 3, 5 }, .weight = 1 },
    };

    // Best routes for each node
    uint32_t const routes[][4] =
    {
        { 1, 2, 0xff },     // 0 -> { 1, 2 }
        { 0, 3, 0xff },     // 1 -> { 0, 3 }
        { 0, 0xff },        // 2 -> { 0 }
        { 1, 4, 5, 0xff },  // 3 -> { 1, 4, 5 }
        { 3, 0xff },        // 4 -> { 3 }
        { 3, 0xff },        // 5 -> { 3 }
    };

    mdv_topology *topology = mdv_test_router_topology_create(nodes, sizeof(nodes) / sizeof *nodes,
                                                             links, sizeof links / sizeof *links);

    for(size_t i = 0; i < sizeof nodes / sizeof *nodes; ++i)
    {
        mdv_hashmap *mstroutes = mdv_routes_find(topology, &nodes[i].uuid);

        uint32_t j = 0;

        for(uint32_t n = routes[i][j]; n != 0xff; n = routes[i][++j])
            mu_check(mdv_hashmap_find(mstroutes, &nodes[routes[i][j]].uuid));

        mu_check(mdv_hashmap_size(mstroutes) == j);

        mdv_hashmap_release(mstroutes);
    }

    mdv_topology_release(topology);
}


void mdv_platform_router_test_2()
{
    /*
        Topology:
              1   4
             / \ /
            0   3
             \ / \
              2   5
    */

    mdv_toponode nodes[] =
    {
        { .id = 1, .uuid = { .u8 = { 0x96, 0xC0, 0x27, 0xC6, 0x03, 0xD9, 0x49, 0xFE, 0x97, 0x1E, 0x1E, 0x4C, 0x9E, 0xCF, 0xD8, 0xF6 } }, .addr = "0" },
        { .id = 2, .uuid = { .u8 = { 0xB9, 0x53, 0x83, 0xF3, 0xCF, 0xAB, 0x44, 0x32, 0x9F, 0x08, 0x42, 0x81, 0x18, 0x01, 0xB6, 0xCA } }, .addr = "1" },
        { .id = 3, .uuid = { .u8 = { 0x58, 0x75, 0xD5, 0xAF, 0x2B, 0xCB, 0x4F, 0x9F, 0x98, 0x75, 0x24, 0xDB, 0xD1, 0xA9, 0x64, 0xAE } }, .addr = "2" },
        { .id = 4, .uuid = { .u8 = { 0xDB, 0xFD, 0x7A, 0xA7, 0x9B, 0x5A, 0x46, 0x4F, 0x99, 0x89, 0x79, 0xB7, 0x74, 0xEA, 0x97, 0x1E } }, .addr = "3" },
        { .id = 5, .uuid = { .u8 = { 0xB7, 0x93, 0x88, 0x9A, 0xBB, 0xA3, 0x46, 0xD1, 0x83, 0x48, 0x5F, 0xD8, 0x50, 0xB2, 0x02, 0xA1 } }, .addr = "4" },
        { .id = 6, .uuid = { .u8 = { 0x93, 0x73, 0x41, 0xBD, 0x3C, 0xBF, 0x4A, 0x71, 0xAF, 0x2A, 0x6F, 0x99, 0xD2, 0xFB, 0x24, 0x0B } }, .addr = "5" },
    };

    mdv_topolink links[] =
    {
        { .node = { 0, 1 }, .weight = 1 },
        { .node = { 0, 2 }, .weight = 1 },
        { .node = { 1, 3 }, .weight = 1 },
        { .node = { 2, 3 }, .weight = 1 },
        { .node = { 3, 4 }, .weight = 1 },
        { .node = { 3, 5 }, .weight = 1 },
    };

    // Best routes for each node
    uint32_t const routes[][5] =
    {
        { 1, 0xff },            // 0 -> { 1 }
        { 0, 3, 0xff },         // 1 -> { 0, 3 }
        { 3, 0xff },            // 2 -> { 3 }
        { 1, 2, 4, 5, 0xff },   // 3 -> { 1, 2, 4, 5 }
        { 3, 0xff },            // 4 -> { 3 }
        { 3, 0xff },            // 5 -> { 3 }
    };

    mdv_topology *topology = mdv_test_router_topology_create(nodes, sizeof(nodes) / sizeof *nodes,
                                                             links, sizeof links / sizeof *links);

    for(size_t i = 0; i < sizeof nodes / sizeof *nodes; ++i)
    {
        mdv_hashmap *mstroutes = mdv_routes_find(topology, &nodes[i].uuid);

        uint32_t j = 0;

        for(uint32_t n = routes[i][j]; n != 0xff; n = routes[i][++j])
            mu_check(mdv_hashmap_find(mstroutes, &nodes[routes[i][j]].uuid));

        mu_check(mdv_hashmap_size(mstroutes) == j);

        mdv_hashmap_release(mstroutes);
    }

    mdv_topology_release(topology);
}


MU_TEST(platform_router)
{
    mdv_platform_router_test_1();
    mdv_platform_router_test_2();
}

