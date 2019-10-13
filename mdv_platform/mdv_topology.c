#include "mdv_topology.h"
#include "mdv_hashmap.h"
#include "mdv_rollbacker.h"
#include "mdv_algorithm.h"
#include "mdv_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>


/// Topology description
struct mdv_topology
{
    atomic_uint_fast32_t    rc;         ///< References counter
    mdv_vector             *nodes;      ///< Nodes array (vector<mdv_toponode>)
    mdv_vector             *links;      ///< Links array (vector<mdv_topolink>)
    mdv_vector             *extradata;  ///< Additional data (place for addresses)
};


mdv_topology mdv_empty_topology =
{
    .nodes = &mdv_empty_vector,
    .links = &mdv_empty_vector,
    .extradata = &mdv_empty_vector
};


mdv_topology_delta mdv_empty_topology_delta =
{
    .ab = &mdv_empty_topology,
    .ba = &mdv_empty_topology
};


int mdv_link_cmp(mdv_topolink const *a, mdv_topolink const *b)
{
    if (a->node[0] < b->node[0])
        return -1;
    else if (a->node[0] > b->node[0])
        return 1;
    else if (a->node[1] < b->node[1])
        return -1;
    else if (a->node[1] > b->node[1])
        return 1;
    return 0;
}


static mdv_vector * mdv_topology_linked_nodes(mdv_vector *topolinks, mdv_vector *toponodes, size_t *extrasize)
{
    *extrasize = 0;

    if (mdv_vector_empty(topolinks))
        return &mdv_empty_vector;

    uint32_t *node_idxs = mdv_stalloc(sizeof(uint32_t) * mdv_vector_size(toponodes), "node_idxs");

    if (!node_idxs)
        return 0;

    memset(node_idxs, 0, sizeof(uint32_t) * mdv_vector_size(toponodes));

    size_t nodes_count = 0;

    mdv_vector_foreach(topolinks, mdv_topolink, link)
    {
        if (!node_idxs[link->node[0]])
        {
            node_idxs[link->node[0]] = 42;
            ++nodes_count;
        }

        if (!node_idxs[link->node[1]])
        {
            node_idxs[link->node[1]] = 42;
            ++nodes_count;
        }
    }

    mdv_vector *linked_nodes = mdv_vector_create(
                                    nodes_count,
                                    sizeof(mdv_toponode),
                                    &mdv_default_allocator);

    if (!linked_nodes)
    {
        mdv_stfree(node_idxs, "node_idxs");
        return 0;
    }

    uint32_t n = 0;

    for(size_t i = 0; i < mdv_vector_size(toponodes); ++i)
    {
        if (node_idxs[i])
        {
            node_idxs[i] = n++;
            mdv_toponode *toponode = mdv_vector_at(toponodes, i);
            mdv_vector_push_back(linked_nodes, toponode);
            *extrasize += strlen(toponode->addr) + 1;
        }
    }

    mdv_vector_foreach(topolinks, mdv_topolink, link)
    {
        link->node[0] = node_idxs[link->node[0]];
        link->node[1] = node_idxs[link->node[1]];
    }

    mdv_stfree(node_idxs, "node_idxs");

    return linked_nodes;
}


mdv_vector * mdv_topology_nodes_extradata(mdv_vector *toponodes, size_t extrasize)
{
    if (!extrasize)
        return &mdv_empty_vector;

    mdv_vector *extradata = mdv_vector_create(
                                extrasize,
                                sizeof(char),
                                &mdv_default_allocator);

    if (!extradata)
        return 0;

    mdv_vector_foreach(toponodes, mdv_toponode, node)
    {
        node->addr = mdv_vector_append(
                            extradata,
                            node->addr,
                            strlen(node->addr) + 1);
    }

    return extradata;
}


mdv_topology_delta * mdv_topology_diff(mdv_topology *a, mdv_topology *b)
{
    if (mdv_vector_empty(a->links)
        && mdv_vector_empty(b->links))
        return &mdv_empty_topology_delta;
    // optimizations begin
    // These lines required for extra memory allocations avoiding.
    // We can remove this optimization if we want more simpler logic.
    else if (mdv_vector_empty(a->links))
    {
        mdv_topology_delta *delta = mdv_alloc(sizeof(mdv_topology_delta), "topology_delta");

        if (!delta)
        {
            MDV_LOGE("no memory for network topology delta");
            return 0;
        }

        delta->ab = &mdv_empty_topology;
        delta->ba = mdv_topology_retain(b);

        return delta;
    }
    else if (mdv_vector_empty(b->links))
    {
        mdv_topology_delta *delta = mdv_alloc(sizeof(mdv_topology_delta), "topology_delta");

        if (!delta)
        {
            MDV_LOGE("no memory for network topology delta");
            return 0;
        }

        delta->ab = mdv_topology_retain(a);
        delta->ba = &mdv_empty_topology;

        return delta;
    }
    // optimizations end

    mdv_vector *ab_links = mdv_vector_clone(a->links, mdv_vector_size(a->links));

    if (!ab_links)
    {
        MDV_LOGE("no memory for network topology delta");
        return 0;
    }

    mdv_vector *ba_links = mdv_vector_clone(b->links, mdv_vector_size(b->links));

    if (!ba_links)
    {
        MDV_LOGE("no memory for network topology delta");
        mdv_vector_release(ab_links);
        return 0;
    }

    // {a} - {b} calculation
    uint32_t const ab_size = mdv_exclude(mdv_vector_data(ab_links), sizeof(mdv_topolink), mdv_vector_size(ab_links),
                                         mdv_vector_data(b->links), sizeof(mdv_topolink), mdv_vector_size(b->links),
                                         (int (*)(const void *a, const void *b))&mdv_link_cmp);

    if (ab_size)
        mdv_vector_resize(ab_links, ab_size);
    else
    {
        mdv_vector_release(ab_links);
        ab_links = &mdv_empty_vector;
    }

    // {b} - {a} calculation
    uint32_t const ba_size = mdv_exclude(mdv_vector_data(ba_links), sizeof(mdv_topolink), mdv_vector_size(ba_links),
                                         mdv_vector_data(a->links), sizeof(mdv_topolink), mdv_vector_size(a->links),
                                         (int (*)(const void *a, const void *b))&mdv_link_cmp);

    if (ba_size)
        mdv_vector_resize(ba_links, ba_size);
    else
    {
        mdv_vector_release(ba_links);
        ba_links = &mdv_empty_vector;
    }

    if (!ab_size && !ba_size)
        return &mdv_empty_topology_delta;

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(8);

    mdv_rollbacker_push(rollbacker, mdv_vector_release, ab_links);
    mdv_rollbacker_push(rollbacker, mdv_vector_release, ba_links);

    // {a} - {b} nodes
    size_t ab_extrasize;
    mdv_vector *ab_nodes = mdv_topology_linked_nodes(ab_links, a->nodes, &ab_extrasize);

    if (!ab_nodes)
    {
        MDV_LOGE("no memory for network topology delta");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, ab_nodes);

    // {b} - {a} nodes
    size_t ba_extrasize;
    mdv_vector *ba_nodes = mdv_topology_linked_nodes(ba_links, b->nodes, &ba_extrasize);

    if (!ba_nodes)
    {
        MDV_LOGE("no memory for network topology delta");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, ba_nodes);

    // {a} - {b} nodes extra data
    mdv_vector *ab_extradata = mdv_topology_nodes_extradata(ab_nodes, ab_extrasize);

    if (!ab_extradata)
    {
        MDV_LOGE("no memory for network topology delta");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, ab_extradata);

    // {b} - {a} nodes extra data
    mdv_vector *ba_extradata = mdv_topology_nodes_extradata(ba_nodes, ba_extrasize);

    if (!ba_extradata)
    {
        MDV_LOGE("no memory for network topology delta");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, ba_extradata);

    // {a} - {b} topology
    mdv_topology *ab_topology = &mdv_empty_topology;

    if (ab_size)
        ab_topology = mdv_topology_create(ab_nodes, ab_links, ab_extradata);

    if (!ab_topology)
    {
        MDV_LOGE("no memory for network topology delta");
        mdv_rollback(rollbacker);
        return 0;
    }

    // {b} - {a} topology
    mdv_topology *ba_topology = &mdv_empty_topology;

    if (ba_size)
        ba_topology = mdv_topology_create(ba_nodes, ba_links, ba_extradata);

    if (!ba_topology)
    {
        MDV_LOGE("no memory for network topology delta");
        mdv_topology_release(ab_topology);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_topology_delta *delta = mdv_alloc(sizeof(mdv_topology_delta), "topology_delta");

    if (!delta)
    {
        MDV_LOGE("no memory for network topology delta");
        mdv_topology_release(ab_topology);
        mdv_topology_release(ba_topology);
        mdv_rollback(rollbacker);
        return 0;
    }

    delta->ab = ab_topology;
    delta->ba = ba_topology;

    mdv_rollback(rollbacker);
}


void mdv_topology_delta_free(mdv_topology_delta *delta)
{
    if (delta && delta != &mdv_empty_topology_delta)
    {
        mdv_topology_release(delta->ab);
        mdv_topology_release(delta->ba);
        mdv_free(delta, "topology_delta");
    }
}


mdv_topology * mdv_topology_create(mdv_vector *nodes,
                                   mdv_vector *links,
                                   mdv_vector *extradata)
{
    mdv_topology *topology = mdv_alloc(sizeof(mdv_topology), "topology");

    if (!topology)
    {
        MDV_LOGE("No memory for network topology");
        return 0;
    }

    topology->nodes = mdv_vector_retain(nodes);
    topology->links = mdv_vector_retain(links);
    topology->extradata = mdv_vector_retain(extradata);

    qsort(mdv_vector_data(links),
          mdv_vector_size(links),
          sizeof(mdv_topolink),
          (int (*)(const void *, const void *))&mdv_link_cmp);

    return topology;
}


static void mdv_topology_free(mdv_topology *topology)
{
    if (topology && topology != &mdv_empty_topology)
    {
        mdv_vector_release(topology->nodes);
        mdv_vector_release(topology->links);
        mdv_vector_release(topology->extradata);
        mdv_free(topology, "topology");
    }
}


mdv_topology * mdv_topology_retain(mdv_topology *topology)
{
    if (topology == &mdv_empty_topology)
        return topology;
    atomic_fetch_add_explicit(&topology->rc, 1, memory_order_acquire);
    return topology;
}


uint32_t mdv_topology_release(mdv_topology *topology)
{
    if (!topology)
        return 0;

    if (topology == &mdv_empty_topology)
        return 1;

    uint32_t rc = atomic_fetch_sub_explicit(&topology->rc, 1, memory_order_release) - 1;

    if (!rc)
        mdv_topology_free(topology);

    return rc;
}


mdv_vector * mdv_topology_nodes(mdv_topology *topology)
{
    return mdv_vector_retain(topology->nodes);
}


mdv_vector * mdv_topology_links(mdv_topology *topology)
{
    return mdv_vector_retain(topology->links);
}


mdv_vector * mdv_topology_extradata(mdv_topology *topology)
{
    return mdv_vector_retain(topology->extradata);
}

