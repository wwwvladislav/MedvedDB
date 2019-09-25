#include "mdv_topology.h"
#include "mdv_tracker.h"
#include "mdv_vector.h"
#include "mdv_hashmap.h"
#include "mdv_rollbacker.h"
#include "mdv_algorithm.h"
#include "mdv_log.h"
#include <stdlib.h>
#include <string.h>


static mdv_topology empty_topology =
{
    .size = sizeof(mdv_topology),
    .nodes_count = 0,
    .links_count = 0
};


static mdv_topology_delta empty_topology_delta =
{
    .ab = &empty_topology,
    .ba = &empty_topology
};


typedef struct
{
    uint32_t        id;
    uint32_t        idx;
    mdv_toponode    node;
} mdv_topology_node;


typedef struct
{
    uint32_t        idx;
    mdv_toponode   *pnode;
} mdv_topology_node_ref;


static size_t mdv_u32_hash(uint32_t const *v)
{
    return *v;
}


static int mdv_u32_keys_cmp(uint32_t const *a, uint32_t const *b)
{
    return (int)*a - *b;
}


static size_t mdv_pnode_hash(mdv_toponode const **v)
{
    return mdv_uuid_hash(&(*v)->uuid);
}


static int mdv_pnode_cmp(mdv_toponode const **a, mdv_toponode const **b)
{
    return mdv_uuid_cmp(&(*a)->uuid, &(*b)->uuid);
}


typedef struct
{
    mdv_tracker     *tracker;
    mdv_vector      *links;         ///< vector<mdv_tracker_link>
    mdv_hashmap      unique_nodes;
} mdv_tracker_links_tmp;


static mdv_topology_node const *mdv_topology_node_register(mdv_tracker_links_tmp *tmp, uint32_t id)
{
    mdv_topology_node const *toponode = mdv_hashmap_find(tmp->unique_nodes, id);

    if (!toponode)
    {
        mdv_node *node = mdv_tracker_node_by_id(tmp->tracker, id);

        if (!node)
        {
            MDV_LOGE("Node with %u identifier wasn't registered", id);
            return 0;
        }

        size_t const addr_size = strlen(node->addr) + 1;
        size_t const size = sizeof(mdv_topology_node) + addr_size;

        char buf[size];

        mdv_topology_node *new_toponode = (void*)buf;

        char *addr_dst = (char*)(new_toponode + 1);

        new_toponode->id = id;
        new_toponode->node.uuid = node->uuid;
        new_toponode->node.addr = addr_dst;

        memcpy(addr_dst, node->addr, addr_size);

        mdv_list_entry_base *entry = _mdv_hashmap_insert(&tmp->unique_nodes, new_toponode, size);

        if (!entry)
        {
            MDV_LOGE("No memory for node id");
            return 0;
        }

        new_toponode = (void*)entry->data;
        new_toponode->node.addr = (char*)(new_toponode + 1);

        toponode = new_toponode;
    }

    return toponode;
}


static void mdv_tracker_link_add(mdv_tracker_link const *link, void *arg)
{
    mdv_tracker_links_tmp *tmp = arg;

    if (mdv_vector_push_back(tmp->links, link))
    {
        mdv_topology_node const *id0 = mdv_topology_node_register(tmp, link->id[0]);
        mdv_topology_node const *id1 = mdv_topology_node_register(tmp, link->id[1]);

        if (id0 && id1)
            return;
    }

    MDV_LOGE("No memory for network topology link");
}


int mdv_link_cmp(mdv_topolink const *a, mdv_topolink const *b)
{
    if (mdv_uuid_cmp(&a->node[0]->uuid, &b->node[0]->uuid) < 0)
        return -1;
    else if (mdv_uuid_cmp(&a->node[0]->uuid, &b->node[0]->uuid) > 0)
        return 1;
    else if (mdv_uuid_cmp(&a->node[1]->uuid, &b->node[1]->uuid) < 0)
        return -1;
    else if (mdv_uuid_cmp(&a->node[1]->uuid, &b->node[1]->uuid) > 0)
        return 1;
    return 0;
}


static size_t mdv_topology_size_by_node_ids(mdv_hashmap const *node_ids, size_t links_count)
{
    size_t toposize = sizeof(mdv_topology)
                        + sizeof(mdv_toponode) * mdv_hashmap_size(*node_ids)
                        + sizeof(mdv_topolink) * links_count;

    mdv_hashmap_foreach(*node_ids, mdv_topology_node, entry)
    {
        toposize += strlen(entry->node.addr) + 1;
    }

    return toposize;
}


static size_t mdv_topology_size_by_node_refs(mdv_hashmap const *node_refs, size_t links_count)
{
    size_t toposize = sizeof(mdv_topology)
                        + sizeof(mdv_toponode) * mdv_hashmap_size(*node_refs)
                        + sizeof(mdv_topolink) * links_count;

    mdv_hashmap_foreach(*node_refs, mdv_topology_node_ref, entry)
    {
        toposize += strlen(entry->pnode->addr) + 1;
    }

    return toposize;
}


static void mdv_topology_copy(mdv_topology *dst, mdv_topology const *src)
{
    dst->size        = src->size;
    dst->nodes_count = src->nodes_count;
    dst->links_count = src->links_count;

    char *strs_space = (char *)(dst->links + dst->links_count);
    char const *strs_space_end = (char const*)dst + dst->size;

    for(size_t i = 0; i < src->nodes_count; ++i)
    {
        dst->nodes[i].uuid = src->nodes[i].uuid;
        dst->nodes[i].addr = strs_space;

        size_t const addr_size = strlen(src->nodes[i].addr) + 1;

        if (strs_space + addr_size > strs_space_end)
        {
            MDV_LOGF("Invalid network topology size");
            break;
        }

        memcpy(strs_space, src->nodes[i].addr, addr_size);

        strs_space += addr_size;
    }

    for(size_t i = 0; i < src->links_count; ++i)
    {
        size_t const s = src->links[i].node[0] - src->nodes;
        size_t const d = src->links[i].node[1] - src->nodes;
        dst->links[i].node[0] = dst->nodes + s;
        dst->links[i].node[1] = dst->nodes + d;
        dst->links[i].weight = src->links[i].weight;
    }
}


static void mdv_topology_copy2(mdv_topology *dst, size_t topology_size, mdv_hashmap const *node_refs, mdv_topolink const *links, size_t links_count)
{
    dst->size = topology_size;
    dst->nodes_count = mdv_hashmap_size(*node_refs);
    dst->links_count = links_count;

    char *strs_space = (char *)(dst->links + dst->links_count);
    char const *strs_space_end = (char const*)dst + dst->size;

    uint32_t idx = 0;

    mdv_hashmap_foreach(*node_refs, mdv_topology_node_ref, entry)
    {
        dst->nodes[idx].uuid = entry->pnode->uuid;
        dst->nodes[idx].addr = strs_space;
        entry->idx = idx++;

        size_t const addr_size = strlen(entry->pnode->addr) + 1;

        if (strs_space + addr_size > strs_space_end)
        {
            MDV_LOGF("Invalid network topology size");
            break;
        }

        memcpy(strs_space, entry->pnode->addr, addr_size);

        strs_space += addr_size;
    }

    for(size_t i = 0; i < links_count; ++i)
    {
        mdv_topology_node_ref const *node_ref_0 = mdv_hashmap_find(*node_refs, links[i].node[0]);
        mdv_topology_node_ref const *node_ref_1 = mdv_hashmap_find(*node_refs, links[i].node[1]);
        dst->links[i].node[0] = dst->nodes + node_ref_0->idx;
        dst->links[i].node[1] = dst->nodes + node_ref_1->idx;
        dst->links[i].weight = links[i].weight;
    }
 }


static bool mdv_topology_unique_nodes_get(mdv_hashmap *node_refs, mdv_topolink const *links, size_t links_count)
{
    if (!mdv_hashmap_init(*node_refs, mdv_topology_node_ref, pnode, 64, mdv_pnode_hash, mdv_pnode_cmp))
    {
        MDV_LOGE("no memory for network topology delta");
        return false;
    }

    for(size_t i = 0; i < links_count; ++i)
    {
        if (!mdv_hashmap_find(*node_refs, links[i].node[0]))
        {
            mdv_topology_node_ref const n =
            {
                .pnode = links[i].node[0]
            };

            if (!mdv_hashmap_insert(*node_refs, n))
            {
                MDV_LOGE("no memory for network topology delta");
                mdv_hashmap_free(*node_refs);
                return false;
            }
        }

        if (!mdv_hashmap_find(*node_refs, links[i].node[1]))
        {
            mdv_topology_node_ref const n =
            {
                .pnode = links[i].node[1]
            };

            if (!mdv_hashmap_insert(*node_refs, n))
            {
                MDV_LOGE("no memory for network topology delta");
                mdv_hashmap_free(*node_refs);
                return false;
            }
        }
    }

    return true;
}


mdv_topology * mdv_topology_extract(mdv_tracker *tracker)
{
    size_t const links_count = mdv_tracker_links_count(tracker);

    if (!links_count)
        return &empty_topology;

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    mdv_tracker_links_tmp tmp =
    {
        .tracker = tracker,
        .links = mdv_vector_create(links_count + 8, sizeof(mdv_tracker_link), &mdv_stallocator)
    };

    if (!tmp.links)
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, tmp.links);

    if (!mdv_hashmap_init(tmp.unique_nodes, mdv_topology_node, id, 64, mdv_u32_hash, mdv_u32_keys_cmp))
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &tmp.unique_nodes);

    mdv_tracker_links_foreach(tracker, &tmp, &mdv_tracker_link_add);

    if (mdv_vector_empty(tmp.links))
    {
        mdv_rollback(rollbacker);
        return &empty_topology;
    }

    size_t const topology_size = mdv_topology_size_by_node_ids(
                                    &tmp.unique_nodes,
                                    mdv_vector_size(tmp.links));

    mdv_topology *topology = mdv_alloc(topology_size, "topology");

    if (!topology)
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, topology, "topology");

    topology->size        = topology_size;
    topology->nodes_count = mdv_hashmap_size(tmp.unique_nodes);
    topology->links_count = mdv_vector_size(tmp.links);
    topology->nodes       = (void*)(topology + 1);
    topology->links       = (void*)(topology->nodes + topology->nodes_count);

    uint32_t n = 0;

    char *strs_space = (char *)(topology->links + topology->links_count);
    char const *strs_space_end = (char const*)topology + topology->size;

    mdv_hashmap_foreach(tmp.unique_nodes, mdv_topology_node, entry)
    {
        entry->idx = n;
        topology->nodes[n].uuid = entry->node.uuid;
        topology->nodes[n].addr = strs_space;

        size_t const addr_size = strlen(entry->node.addr) + 1;

        if (strs_space + addr_size > strs_space_end)
        {
            MDV_LOGE("Invalid network topology size");
            mdv_rollback(rollbacker);
            return 0;
        }

        memcpy(strs_space, entry->node.addr, addr_size);

        strs_space += addr_size;

        ++n;
    }

    n = 0;

    mdv_vector_foreach(tmp.links, mdv_tracker_link, link)
    {
        mdv_topology_node const *id0 = mdv_hashmap_find(tmp.unique_nodes, link->id[0]);
        mdv_topology_node const *id1 = mdv_hashmap_find(tmp.unique_nodes, link->id[1]);

        if (!id0 || !id1)
        {
            MDV_LOGE("Invalid network topology");
            mdv_rollback(rollbacker);
            return 0;
        }

        mdv_topolink *l = topology->links + n;

        l->node[0] = topology->nodes + id0->idx;
        l->node[1] = topology->nodes + id1->idx;
        l->weight = link->weight;

        ++n;
    }

    mdv_hashmap_free(tmp.unique_nodes);
    mdv_vector_release(tmp.links);

    qsort(topology->links,
          topology->links_count,
          sizeof *topology->links,
          (int (*)(const void *, const void *))&mdv_link_cmp);

    mdv_rollbacker_free(rollbacker);

    return topology;
}


void mdv_topology_free(mdv_topology *topology)
{
    if (topology && topology != &empty_topology)
        mdv_free(topology, "topology");
}


mdv_topology_delta * mdv_topology_diff(mdv_topology const *a, mdv_topology const *b)
{
    if (!a->links_count && !b->links_count)
        return &empty_topology_delta;
    // optimizations begin
    // These lines required for extra memory allocations avoiding.
    // We can remove this optimization if we want more simpler logic.
    else if (!a->links_count)
    {
        mdv_topology_delta *delta = mdv_alloc(sizeof(mdv_topology_delta) + b->size, "topology_delta");

        if (!delta)
        {
            MDV_LOGE("no memory for network topology delta");
            return 0;
        }

        delta->ab = &empty_topology;

        delta->ba = (mdv_topology*)(delta + 1);
        delta->ba->nodes = (void*)(delta->ba + 1);
        delta->ba->links = (void*)(delta->ba->nodes + b->nodes_count);
        mdv_topology_copy(delta->ba, b);

        return delta;
    }
    else if (!b->links_count)
    {
        mdv_topology_delta *delta = mdv_alloc(sizeof(mdv_topology_delta) + a->size, "topology_delta");

        if (!delta)
        {
            MDV_LOGE("no memory for network topology delta");
            return 0;
        }

        delta->ab = (mdv_topology*)(delta + 1);
        delta->ab->nodes = (void*)(delta->ab + 1);
        delta->ab->links = (void*)(delta->ab->nodes + a->nodes_count);
        mdv_topology_copy(delta->ab, a);

        delta->ba = &empty_topology;

        return delta;
    }
    // optimizations end

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    mdv_topolink *tmp_links = mdv_staligned_alloc(sizeof(mdv_topolink), sizeof(mdv_topolink) * (a->links_count + b->links_count), "delta_links");

    if (!tmp_links)
    {
        MDV_LOGE("no memory for network topology delta");
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_stfree, tmp_links, "delta_links");

    mdv_topolink *ab_links = tmp_links;
    mdv_topolink *ba_links = tmp_links + a->links_count;

    memcpy(ab_links, a->links, sizeof(mdv_topolink) * a->links_count);
    memcpy(ba_links, b->links, sizeof(mdv_topolink) * b->links_count);

    // {a} - {b} calculation
    uint32_t const ab_size = mdv_exclude(ab_links,  sizeof(mdv_topolink), a->links_count,
                                         b->links, sizeof(mdv_topolink), b->links_count,
                                         (int (*)(const void *a, const void *b))&mdv_link_cmp);

    // {b} - {a} calculation
    uint32_t const ba_size = mdv_exclude(ba_links,  sizeof(mdv_topolink), b->links_count,
                                         a->links, sizeof(mdv_topolink), a->links_count,
                                         (int (*)(const void *a, const void *b))&mdv_link_cmp);

    mdv_hashmap ab_nodes, ba_nodes;     // mdv_topology_node_ref

    if (ab_size)
    {
        if (!mdv_topology_unique_nodes_get(&ab_nodes, ab_links, ab_size))
        {
            mdv_rollback(rollbacker);
            return 0;
        }

        mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &ab_nodes);
    }

    if (ba_size)
    {
        if (!mdv_topology_unique_nodes_get(&ba_nodes, ba_links, ba_size))
        {
            mdv_rollback(rollbacker);
            return 0;
        }

        mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &ba_nodes);
    }

    mdv_topology_delta *delta = 0;

    if (!ab_size && !ba_size)
        delta = &empty_topology_delta;
    else if (!ab_size)
    {
        size_t const topology_size = mdv_topology_size_by_node_refs(&ba_nodes, ba_size);

        delta = mdv_alloc(sizeof(mdv_topology_delta) + topology_size, "topology_delta");

        if (delta)
        {
            delta->ab = &empty_topology;

            delta->ba = (mdv_topology*)(delta + 1);
            delta->ba->nodes_count = mdv_hashmap_size(ba_nodes);
            delta->ba->links_count = ba_size;
            delta->ba->nodes = (void*)(delta->ba + 1);
            delta->ba->links = (void*)(delta->ba->nodes + delta->ba->nodes_count);
            mdv_topology_copy2(delta->ba, topology_size, &ba_nodes, ba_links, ba_size);
        }
        else
            MDV_LOGE("no memory for network topology delta");
    }
    else if (!ba_size)
    {
        size_t const topology_size = mdv_topology_size_by_node_refs(&ab_nodes, ab_size);

        delta = mdv_alloc(sizeof(mdv_topology_delta) + topology_size, "topology_delta");

        if (!delta)
        {
            MDV_LOGE("no memory for network topology delta");
            mdv_rollback(rollbacker);
            return 0;
        }

        delta->ab = (mdv_topology*)(delta + 1);
        delta->ab->nodes_count = mdv_hashmap_size(ab_nodes);
        delta->ab->links_count = ab_size;
        delta->ab->nodes = (void*)(delta->ab + 1);
        delta->ab->links = (void*)(delta->ab->nodes + delta->ab->nodes_count);
        mdv_topology_copy2(delta->ab, topology_size, &ab_nodes, ab_links, ab_size);

        delta->ba = &empty_topology;
    }
    else
    {
        size_t const ab_topology_size = mdv_topology_size_by_node_refs(&ab_nodes, ab_size);
        size_t const ba_topology_size = mdv_topology_size_by_node_refs(&ba_nodes, ba_size);

        delta = mdv_alloc(sizeof(mdv_topology_delta) + ab_topology_size + ba_topology_size, "topology_delta");
        if (!delta)
        {
            MDV_LOGE("no memory for network topology delta");
            mdv_rollback(rollbacker);
            return 0;
        }

        delta->ab = (mdv_topology*)(delta + 1);
        delta->ab->nodes_count = mdv_hashmap_size(ab_nodes);
        delta->ab->links_count = ab_size;
        delta->ab->nodes = (void*)(delta->ab + 1);
        delta->ab->links = (void*)(delta->ab->nodes + delta->ab->nodes_count);

        delta->ba = (mdv_topology*)((char*)delta->ab + ab_topology_size);
        delta->ba->nodes_count = mdv_hashmap_size(ba_nodes);
        delta->ba->links_count = ba_size;
        delta->ba->nodes = (void*)(delta->ba + 1);
        delta->ba->links = (void*)(delta->ba->nodes + delta->ba->nodes_count);

        mdv_topology_copy2(delta->ab, ab_topology_size, &ab_nodes, ab_links, ab_size);
        mdv_topology_copy2(delta->ba, ba_topology_size, &ba_nodes, ba_links, ba_size);
    }

    mdv_rollback(rollbacker);

    return delta;
}


void mdv_topology_delta_free(mdv_topology_delta *delta)
{
    if (delta && delta != &empty_topology_delta)
        mdv_free(delta, "topology_delta");
}
