#include "mdv_topology.h"
#include "mdv_hashmap.h"
#include "mdv_rollbacker.h"
#include "mdv_algorithm.h"
#include "mdv_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>


typedef int (*mdv_qsort_comparer) (const void *, const void *);


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


static int mdv_node_cmp(mdv_toponode const *a, mdv_toponode const *b)
{
    return mdv_uuid_cmp(&a->uuid, &b->uuid);
}


static bool mdv_node_equ(void const *a, void const *b)
{
    return mdv_node_cmp(a, b) == 0;
}


static mdv_vector * mdv_topology_linked_nodes(mdv_vector *topolinks, mdv_vector *toponodes, size_t *extrasize)
{
    *extrasize = 0;

    if (mdv_vector_empty(topolinks))
        return mdv_vector_create(1,
                                 sizeof(mdv_toponode),
                                 &mdv_default_allocator);

    uint32_t *node_idxs = mdv_alloc(sizeof(uint32_t) * mdv_vector_size(toponodes), "node_idxs");

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
                                    nodes_count + 1,
                                    sizeof(mdv_toponode),
                                    &mdv_default_allocator);

    if (!linked_nodes)
    {
        mdv_free(node_idxs, "node_idxs");
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

    mdv_free(node_idxs, "node_idxs");

    return linked_nodes;
}


static bool mdv_topology_linked_node_append(mdv_vector *topolinks,
                                            mdv_vector *toponodes,
                                            size_t *extrasize,
                                            mdv_toponode const *lnodes[2],
                                            uint32_t lweight)
{
    if (!lnodes[0] || !lnodes[1])
    {
        MDV_LOGE("Invalid link");
        return false;
    }

    uint32_t lnode_idx = 0;
    uint32_t rnode_idx = 0;

    mdv_toponode const *lnode = mdv_vector_find(toponodes, lnodes[0], mdv_node_equ);
    if (lnode)
        lnode_idx = lnode - (mdv_toponode const *)mdv_vector_data(toponodes);

    mdv_toponode const *rnode = mdv_vector_find(toponodes, lnodes[1], mdv_node_equ);
    if (rnode)
        rnode_idx = rnode - (mdv_toponode const *)mdv_vector_data(toponodes);

    if(!lnode)
    {
        lnode_idx = mdv_vector_size(toponodes);
        if (!mdv_vector_push_back(toponodes, lnodes[0]))
            return false;
        *extrasize += strlen(lnodes[0]->addr) + 1;
    }

    if(!rnode)
    {
        rnode_idx = mdv_vector_size(toponodes);
        if (!mdv_vector_push_back(toponodes, lnodes[1]))
            return false;
        *extrasize += strlen(lnodes[1]->addr) + 1;
    }

    mdv_topolink const link =
    {
        .node = { lnode_idx, rnode_idx },
        .weight = lweight
    };

    return mdv_vector_push_back(topolinks, &link);
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


typedef struct
{
    mdv_uuid const *node[2];
} mdv_utopolink;


static mdv_utopolink mdv_uuids_sort(mdv_uuid const *node[2])
{
    mdv_utopolink res;

    if (mdv_uuid_cmp(node[0], node[1]) <= 0)
    {
        res.node[0] = node[0];
        res.node[1] = node[1];
    }
    else
    {
        res.node[0] = node[1];
        res.node[1] = node[0];
    }

    return res;
}


static int mdv_utopolink_cmp(mdv_uuid const *a[2], mdv_uuid const *b[2])
{
    mdv_utopolink aa = mdv_uuids_sort(a);
    mdv_utopolink bb = mdv_uuids_sort(b);

    int const cmp1 = mdv_uuid_cmp(aa.node[0], bb.node[0]);

    if (cmp1 < 0)
        return -1;
    else if (cmp1 > 0)
        return 1;

    int const cmp2 = mdv_uuid_cmp(aa.node[1], bb.node[1]);

    if (cmp2 < 0)
        return -1;
    else if (cmp2 > 0)
        return 1;

    return 0;
}


static size_t mdv_utopolink_hash(mdv_uuid const *a[2])
{
    mdv_utopolink aa = mdv_uuids_sort(a);
    return mdv_uuid_hash(aa.node[0]) * 37
            + mdv_uuid_hash(aa.node[1]);
}


mdv_hashmap * mdv_topology_links_map(mdv_topology *topology)
{
    mdv_hashmap *blinks = mdv_hashmap_create(
                                mdv_utopolink,
                                node,
                                mdv_vector_size(topology->links) * 5 / 3,
                                mdv_utopolink_hash,
                                mdv_utopolink_cmp);

    if (!blinks)
    {
        MDV_LOGE("No memory for links");
        return 0;
    }

    mdv_vector_foreach(topology->links, mdv_topolink, link)
    {
        mdv_toponode const *lnode = mdv_vector_at(topology->nodes, link->node[0]);
        mdv_toponode const *rnode = mdv_vector_at(topology->nodes, link->node[1]);

        mdv_utopolink const l =
        {
            .node =
            {
                &lnode->uuid,
                &rnode->uuid
            }
        };

        if (!mdv_hashmap_insert(blinks, &l, sizeof l))
        {
            MDV_LOGE("No memory for links");
            mdv_hashmap_release(blinks);
            return 0;
        }
    }

    return blinks;
}


mdv_topology * mdv_topology_diff(mdv_topology *a, mdv_uuid const *nodea,
                                 mdv_topology *b, mdv_uuid const *nodeb)
{
    if (mdv_vector_empty(a->links))     // a -> { b, ... }
        return &mdv_empty_topology;

    if (mdv_vector_empty(b->links))     // { a, ... } -> b
        return mdv_topology_retain(a);

    // { a, ... } -> { b, ... }

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    mdv_hashmap *blinks = mdv_topology_links_map(b);

    if (!blinks)
    {
        MDV_LOGE("No memory for network topology delta");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, blinks);

    mdv_vector *ab_links = mdv_vector_create(mdv_vector_size(a->links) + 1,
                                             sizeof(mdv_topolink),
                                             &mdv_default_allocator);

    if (!ab_links)
    {
        MDV_LOGE("no memory for network topology delta");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, ab_links);

    bool ab_link_added = false;

    mdv_toponode const *lnodes[2] = { 0, 0 };
    uint32_t lweight = 0;

    // {a} - {b} calculation
    mdv_vector_foreach(a->links, mdv_topolink, link)
    {
        mdv_toponode const *lnode = mdv_vector_at(a->nodes, link->node[0]);
        mdv_toponode const *rnode = mdv_vector_at(a->nodes, link->node[1]);

        mdv_utopolink const l =
        {
            .node =
            {
                &lnode->uuid,
                &rnode->uuid,
            }
        };

        if (!ab_link_added)
        {
            if ((mdv_uuid_cmp(nodea, &lnode->uuid) == 0 && mdv_uuid_cmp(nodeb, &rnode->uuid) == 0) ||
                (mdv_uuid_cmp(nodea, &rnode->uuid) == 0 && mdv_uuid_cmp(nodeb, &lnode->uuid) == 0))
            {
                mdv_vector_push_back(ab_links, link);
                ab_link_added = true;
                lnodes[0] = lnode;
                lnodes[1] = rnode;
                lweight = link->weight;
                continue;
            }
        }

        if (!mdv_hashmap_find(blinks, &l.node))
            mdv_vector_push_back(ab_links, link);
    }

    if (!ab_link_added)
    {
        mdv_vector_foreach(b->links, mdv_topolink, link)
        {
            mdv_toponode const *lnode = mdv_vector_at(b->nodes, link->node[0]);
            mdv_toponode const *rnode = mdv_vector_at(b->nodes, link->node[1]);

            if ((mdv_uuid_cmp(nodea, &lnode->uuid) == 0 && mdv_uuid_cmp(nodeb, &rnode->uuid) == 0) ||
                (mdv_uuid_cmp(nodea, &rnode->uuid) == 0 && mdv_uuid_cmp(nodeb, &lnode->uuid) == 0))
            {
                lnodes[0] = lnode;
                lnodes[1] = rnode;
                lweight = link->weight;
                break;
            }
        }
    }

    uint32_t const ab_size = mdv_vector_size(ab_links);

    if (!ab_size && !lnodes[0])
    {
        mdv_rollback(rollbacker);
        return &mdv_empty_topology;
    }

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

    if (!ab_link_added
        && !mdv_topology_linked_node_append(ab_links, ab_nodes, &ab_extrasize, lnodes, lweight))
    {
        MDV_LOGE("no memory for network topology delta");
        mdv_rollback(rollbacker);
        return 0;
    }

    // {a} - {b} nodes extra data
    mdv_vector *ab_extradata = mdv_topology_nodes_extradata(ab_nodes, ab_extrasize);

    if (!ab_extradata)
    {
        MDV_LOGE("no memory for network topology delta");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, ab_extradata);

    // {a} - {b} topology
    mdv_topology *ab_topology = mdv_topology_create(ab_nodes, ab_links, ab_extradata);

    if (!ab_topology)
    {
        MDV_LOGE("no memory for network topology delta");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollback(rollbacker);

    return ab_topology;
}


typedef struct
{
    uint32_t id;
    uint32_t idx;
} mdv_node_idx;


static size_t mdv_u32_hash(uint32_t const *id)
{
    return *id;
}

static int mdv_u32_cmp(uint32_t const *id1, uint32_t const *id2)
{
    return (int)*id1 - *id2;
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

    mdv_hashmap *node_idxs = mdv_hashmap_create(mdv_node_idx, id, mdv_vector_size(nodes), mdv_u32_hash, mdv_u32_cmp);

    if (!node_idxs)
    {
        MDV_LOGE("No memory for network topology");
        mdv_free(topology, "topology");
        return 0;
    }

    atomic_init(&topology->rc, 1);

    mdv_vector_foreach(links, mdv_topolink, link)
    {
        mdv_toponode *node0 = mdv_vector_at(nodes, link->node[0]);
        mdv_toponode *node1 = mdv_vector_at(nodes, link->node[1]);
        link->node[0] = node0->id;
        link->node[1] = node1->id;
    }

    qsort(mdv_vector_data(nodes),
          mdv_vector_size(nodes),
          sizeof(mdv_toponode),
          (mdv_qsort_comparer)mdv_node_cmp);

    // restore linked node indices

    uint32_t n = 0;

    mdv_vector_foreach(nodes, mdv_toponode, node)
    {
        mdv_node_idx const idx =
        {
            .id = node->id,
            .idx = n++
        };

        mdv_hashmap_insert(node_idxs, &idx, sizeof idx);
    }

    mdv_vector_foreach(links, mdv_topolink, link)
    {
        mdv_node_idx const *node0 = mdv_hashmap_find(node_idxs, &link->node[0]);
        mdv_node_idx const *node1 = mdv_hashmap_find(node_idxs, &link->node[1]);
        link->node[0] = node0->idx;
        link->node[1] = node1->idx;
    }

    mdv_hashmap_release(node_idxs);

    topology->nodes = mdv_vector_retain(nodes);
    topology->links = mdv_vector_retain(links);
    topology->extradata = mdv_vector_retain(extradata);

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


mdv_hashmap * mdv_topology_peers(mdv_topology *topology, mdv_uuid const *node)
{
    mdv_hashmap *peers = _mdv_hashmap_create(
                                4,
                                0,
                                sizeof(mdv_uuid),
                                (mdv_hash_fn)mdv_uuid_hash,
                                (mdv_key_cmp_fn)mdv_uuid_cmp);

    if (peers)
    {
        mdv_vector_foreach(topology->links, mdv_topolink, link)
        {
            mdv_toponode const *lnode = mdv_vector_at(topology->nodes, link->node[0]);
            mdv_toponode const *rnode = mdv_vector_at(topology->nodes, link->node[1]);

            if (mdv_uuid_cmp(node, &lnode->uuid) == 0)
            {
                if (!mdv_hashmap_insert(peers, &rnode->uuid, sizeof(mdv_uuid)))
                {
                    MDV_LOGE("No memory for peers");
                    mdv_hashmap_release(peers);
                    return 0;
                }
            }
            else if (mdv_uuid_cmp(node, &rnode->uuid) == 0)
            {
                if (!mdv_hashmap_insert(peers, &lnode->uuid, sizeof(mdv_uuid)))
                {
                    MDV_LOGE("No memory for peers");
                    mdv_hashmap_release(peers);
                    return 0;
                }
            }
        }
    }

    return peers;
}
