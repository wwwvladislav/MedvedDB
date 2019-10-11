#include "mdv_topology.h"
#include "mdv_vector.h"
#include "mdv_hashmap.h"
#include "mdv_rollbacker.h"
#include "mdv_algorithm.h"
#include "mdv_log.h"
#include <string.h>


mdv_topology empty_topology =
{
    .size = sizeof(mdv_topology),
    .nodes_count = 0,
    .links_count = 0
};


mdv_topology_delta empty_topology_delta =
{
    .ab = &empty_topology,
    .ba = &empty_topology
};


typedef struct
{
    uint32_t        idx;
    mdv_toponode   *pnode;
} mdv_topology_node_ref;


static size_t mdv_pnode_hash(mdv_toponode const **v)
{
    return mdv_uuid_hash(&(*v)->uuid);
}


static int mdv_pnode_cmp(mdv_toponode const **a, mdv_toponode const **b)
{
    return mdv_uuid_cmp(&(*a)->uuid, &(*b)->uuid);
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


void mdv_topology_free(mdv_topology *topology)
{
    if (topology && topology != &empty_topology)
        mdv_free(topology, "topology");
}
