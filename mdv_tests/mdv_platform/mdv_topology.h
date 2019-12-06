#pragma once
#include "../minunit.h"
#include <mdv_topology.h>
#include <stdint.h>


static mdv_topology * mdv_test_topology_create(mdv_toponode *nodes, size_t nsize,
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


static bool mdv_test_toponode_equ(void const *a, void const *b)
{
    mdv_toponode const *lnode = a;
    mdv_toponode const *rnode = b;
    return mdv_uuid_cmp(&lnode->uuid, &rnode->uuid) == 0;
}


static bool mdv_test_topolink_equ(void const *a, void const *b)
{
    mdv_topolink const *llink = a;
    mdv_topolink const *rlink = b;
    return mdv_link_cmp(llink, rlink) == 0
            && llink->weight == rlink->weight;
}


static void mdv_topology_check(mdv_topology *t,
                               mdv_toponode *nodes, size_t nsize,
                               mdv_topolink *links, size_t lsize)
{
    mdv_vector *toponodes = mdv_topology_nodes(t);
    mdv_vector *topolinks = mdv_topology_links(t);

    mu_check(mdv_vector_size(toponodes) == nsize);

    for(size_t i = 0; i < nsize; ++i)
    {
        mu_check(mdv_vector_find(
                        toponodes,
                        nodes + i,
                        mdv_test_toponode_equ));
    }

    mu_check(mdv_vector_size(topolinks) == lsize);

    for(size_t i = 0; i < lsize; ++i)
    {
        mu_check(mdv_vector_find(
                        topolinks,
                        links + i,
                        mdv_test_topolink_equ));
    }

    mdv_vector_release(toponodes);
    mdv_vector_release(topolinks);
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
        Link: 3 -> 4
    */

    mdv_toponode nodes[] =
    {
        { .id = 1, .uuid = { .a = 1 }, .addr = "1" },
        { .id = 2, .uuid = { .a = 2 }, .addr = "2" },
        { .id = 3, .uuid = { .a = 3 }, .addr = "3" },
        { .id = 4, .uuid = { .a = 4 }, .addr = "4" },
    };

    mdv_topolink a_links[] =
    {
        { .node = { 0, 1 }, .weight = 1 },
        { .node = { 0, 2 }, .weight = 1 },
    };

    mdv_topolink b_links[] =
    {
        { .node = { 0, 1 }, .weight = 1 },
        { .node = { 0, 3 }, .weight = 1 },
        { .node = { 2, 3 }, .weight = 1 },
    };

    mdv_toponode ab_nodes[] =
    {
        { .id = 1, .uuid = { .a = 1 }, .addr = "1" },
        { .id = 3, .uuid = { .a = 3 }, .addr = "3" },
        { .id = 4, .uuid = { .a = 4 }, .addr = "4" },
    };

    mdv_topolink ab_links[] =
    {
        { .node = { 0, 1 }, .weight = 1 },
        { .node = { 1, 2 }, .weight = 1 },
    };

    mdv_toponode ba_nodes[] =
    {
        { .id = 1, .uuid = { .a = 1 }, .addr = "1" },
        { .id = 3, .uuid = { .a = 3 }, .addr = "3" },
        { .id = 4, .uuid = { .a = 4 }, .addr = "4" },
    };

    mdv_topolink ba_links[] =
    {
        { .node = { 0, 2 }, .weight = 1 },
        { .node = { 1, 2 }, .weight = 1 },
    };

    mdv_topology *a = mdv_test_topology_create(nodes,   sizeof(nodes) / sizeof *nodes,
                                               a_links, sizeof a_links / sizeof *a_links);

    mdv_topology *b = mdv_test_topology_create(nodes,   sizeof(nodes) / sizeof *nodes,
                                               b_links, sizeof b_links / sizeof *b_links);

    mdv_topology *ab_delta = mdv_topology_diff(a, &nodes[2].uuid,
                                               b, &nodes[3].uuid);
    mdv_topology *ba_delta = mdv_topology_diff(b, &nodes[3].uuid,
                                               a, &nodes[2].uuid);

    mdv_topology_check(ab_delta,
                       ab_nodes, sizeof ab_nodes / sizeof *ab_nodes,
                       ab_links, sizeof ab_links / sizeof *ab_links);
    mdv_topology_check(ba_delta,
                       ba_nodes, sizeof ba_nodes / sizeof *ba_nodes,
                       ba_links, sizeof ba_links / sizeof *ba_links);

    mdv_topology_release(a);
    mdv_topology_release(b);

    mdv_topology_release(ab_delta);
    mdv_topology_release(ba_delta);
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
        Link: 1 -> 2
    */

    mdv_toponode nodes[] =
    {
        { .id = 1, .uuid = { .a = 1 }, .addr = "1" },
        { .id = 2, .uuid = { .a = 2 }, .addr = "2" },
        { .id = 3, .uuid = { .a = 3 }, .addr = "3" },
    };

    mdv_topolink a_links[] =
    {
        { .node = { 0, 1 }, .weight = 1 },
        { .node = { 0, 2 }, .weight = 1 },
    };

    mdv_topolink b_links[] =
    {
        { .node = { 0, 1 }, .weight = 1 },
    };

    mdv_toponode ab_nodes[] =
    {
        { .id = 1, .uuid = { .a = 1 }, .addr = "1" },
        { .id = 2, .uuid = { .a = 2 }, .addr = "2" },
        { .id = 3, .uuid = { .a = 3 }, .addr = "3" },
    };

    mdv_topolink ab_links[] =
    {
        { .node = { 0, 1 }, .weight = 1 },
        { .node = { 0, 2 }, .weight = 1 },
    };

    mdv_toponode ba_nodes[] =
    {
        { .id = 1, .uuid = { .a = 1 }, .addr = "1" },
        { .id = 2, .uuid = { .a = 2 }, .addr = "2" },
    };

    mdv_topolink ba_links[] =
    {
        { .node = { 0, 1 }, .weight = 1 },
    };

    mdv_topology *a = mdv_test_topology_create(nodes,   sizeof(nodes) / sizeof *nodes,
                                               a_links, sizeof a_links / sizeof *a_links);

    mdv_topology *b = mdv_test_topology_create(nodes,   sizeof(nodes) / sizeof *nodes,
                                               b_links, sizeof b_links / sizeof *b_links);

    mdv_topology *ab_delta = mdv_topology_diff(a, &nodes[0].uuid,
                                               b, &nodes[1].uuid);
    mdv_topology *ba_delta = mdv_topology_diff(b, &nodes[1].uuid,
                                               a, &nodes[0].uuid);

    mdv_topology_check(ab_delta,
                       ab_nodes, sizeof ab_nodes / sizeof *ab_nodes,
                       ab_links, sizeof ab_links / sizeof *ab_links);
    mdv_topology_check(ba_delta,
                       ba_nodes, sizeof ba_nodes / sizeof *ba_nodes,
                       ba_links, sizeof ba_links / sizeof *ba_links);

    mdv_topology_release(a);
    mdv_topology_release(b);

    mdv_topology_release(ab_delta);
    mdv_topology_release(ba_delta);
}


static void mdv_topology_test_3()
{
    /*
        Topology 'a':
            1 - 2
        Topology 'b':
            1 - 2
        Link: 1 -> 2
    */

    mdv_toponode nodes[] =
    {
        { .id = 1, .uuid = { .a = 1 }, .addr = "1" },
        { .id = 2, .uuid = { .a = 2 }, .addr = "2" },
    };

    mdv_topolink links[] =
    {
        { .node = { 0, 1 }, .weight = 1 },
    };

    mdv_topology *a = mdv_test_topology_create(nodes, sizeof(nodes) / sizeof *nodes,
                                               links, sizeof links / sizeof *links);

    mdv_topology *b = mdv_test_topology_create(nodes, sizeof(nodes) / sizeof *nodes,
                                               links, sizeof links / sizeof *links);

    mdv_topology *ab_delta = mdv_topology_diff(a, &nodes[0].uuid,
                                               b, &nodes[1].uuid);
    mdv_topology *ba_delta = mdv_topology_diff(b, &nodes[1].uuid,
                                               a, &nodes[0].uuid);

    mdv_topology_check(ab_delta,
                       nodes, sizeof nodes / sizeof *nodes,
                       links, sizeof links / sizeof *links);
    mdv_topology_check(ba_delta,
                       nodes, sizeof nodes / sizeof *nodes,
                       links, sizeof links / sizeof *links);

    mdv_topology_release(a);
    mdv_topology_release(b);

    mdv_topology_release(ab_delta);
    mdv_topology_release(ba_delta);
}


MU_TEST(platform_topology)
{
    mdv_topology_test_1();
    mdv_topology_test_2();
    mdv_topology_test_3();
}

