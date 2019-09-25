#include "mdv_router.h"
#include "mdv_rollbacker.h"
#include "mdv_mst.h"
#include "mdv_log.h"
#include "mdv_limits.h"
#include <stdlib.h>


typedef struct
{
    uint32_t id;
    uint32_t idx;
    mdv_uuid uuid;
} mdv_router_node;


typedef struct
{
    mdv_tracker     *tracker;
    mdv_hashmap      unique_nodes;
    mdv_vector      *links;             ///< vector<mdv_tracker_link>
} mdv_router_topology;


static size_t mdv_u32_hash(uint32_t const *v)
{
    return *v;
}


static int mdv_u32_keys_cmp(uint32_t const *a, uint32_t const *b)
{
    return (int)*a - *b;
}


static mdv_router_node const *mdv_router_node_register(mdv_router_topology *topology, uint32_t id)
{
    mdv_router_node const *toponode = mdv_hashmap_find(topology->unique_nodes, id);

    if (!toponode)
    {
        mdv_node *node = mdv_tracker_node_by_id(topology->tracker, id);

        if (!node)
        {
            MDV_LOGE("Node with %u identifier wasn't registered", id);
            return 0;
        }

        mdv_router_node new_toponode =
        {
            .id = id,
            .uuid = node->uuid
        };

        mdv_list_entry_base *entry = mdv_hashmap_insert(topology->unique_nodes, new_toponode);

        if (!entry)
        {
            MDV_LOGE("No memory for node id");
            return 0;
        }

        toponode = (void*)entry->data;
    }

    return toponode;
}


static int mdv_router_node_cmp(mdv_router_node const **a, mdv_router_node const **b)
{
    return mdv_uuid_cmp(&(*a)->uuid, &(*b)->uuid);
}


static void mdv_router_link_add(mdv_tracker_link const *link, void *arg)
{
    mdv_router_topology *topology = arg;

    if (mdv_vector_push_back(topology->links, link))
    {
        mdv_router_node const *n1 = mdv_router_node_register(topology, link->id[0]);
        mdv_router_node const *n2 = mdv_router_node_register(topology, link->id[1]);

        if (n1 && n2)
            return;
    }

    MDV_LOGE("No memory for network topology link");
}


static bool mdv_router_topology_create(mdv_router_topology *topology, mdv_tracker *tracker, size_t links_count)
{
    topology->tracker = tracker;

    topology->links = mdv_vector_create(links_count + 8, sizeof(mdv_tracker_link), &mdv_stallocator);

    if (!topology->links)
    {
        MDV_LOGE("No memorty for routes");
        return MDV_NO_MEM;
    }

    if (!mdv_hashmap_init(topology->unique_nodes,
                          mdv_router_node,
                          id,
                          64,
                          mdv_u32_hash,
                          mdv_u32_keys_cmp))
    {
        MDV_LOGE("No memory for network topology");
        mdv_vector_release(topology->links);
        return MDV_NO_MEM;
    }

    mdv_tracker_links_foreach(tracker, topology, &mdv_router_link_add);

    mdv_router_node **pnodes = mdv_stalloc(mdv_hashmap_size(topology->unique_nodes) * sizeof(mdv_router_node *), "router.pnodes");

    if (!pnodes)
    {
        mdv_vector_release(topology->links);
        mdv_hashmap_free(topology->unique_nodes);
        return MDV_NO_MEM;
    }

    uint32_t i = 0;

    mdv_hashmap_foreach(topology->unique_nodes, mdv_router_node, node)
    {
        pnodes[i++] = node;
    }

    qsort(pnodes,
          mdv_hashmap_size(topology->unique_nodes),
          sizeof(mdv_router_node *),
          (int (*)(const void *, const void *))&mdv_router_node_cmp);

    for(uint32_t idx = 0; idx < mdv_hashmap_size(topology->unique_nodes); ++idx)
        pnodes[idx]->idx = idx;

    mdv_stfree(pnodes, "router.pnodes");

    return true;
}


static void mdv_router_topology_free(mdv_router_topology *topology)
{
    mdv_vector_release(topology->links);
    mdv_hashmap_free(topology->unique_nodes);
}


mdv_vector * mdv_routes_find(mdv_tracker *tracker)
{
    size_t const links_count = mdv_tracker_links_count(tracker);

    if (!links_count)
        return mdv_empty_vector;

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    mdv_router_topology topology;

    if (!mdv_router_topology_create(&topology, tracker, links_count))
    {
        MDV_LOGE("No memorty for routes");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_router_topology_free, &topology);

    mdv_mstnode *mst_nodes = mdv_stalloc(mdv_hashmap_size(topology.unique_nodes) * sizeof(mdv_mstnode), "mst_nodes");

    if (!mst_nodes)
    {
        MDV_LOGE("No memorty for MST nodes");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_stfree, mst_nodes, "mst_nodes");

    mdv_hashmap_foreach(topology.unique_nodes, mdv_router_node, node)
    {
        mst_nodes[node->idx].data = (void*)(size_t)node->id;
    }

    mdv_mstlink *mst_links = mdv_stalloc(mdv_vector_size(topology.links) * sizeof(mdv_mstlink), "mst_links");

    if (!mst_links)
    {
        MDV_LOGE("No memorty for MST links");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_stfree, mst_links, "mst_links");

    for(size_t i = 0; i < mdv_vector_size(topology.links); ++i)
    {
        mdv_tracker_link *link = mdv_vector_at(topology.links, i);

        uint32_t weight = link->weight > MDV_LINK_WEIGHT_MAX
                            ? MDV_LINK_WEIGHT_MAX:
                            link->weight;

        mst_links[i].weight = MDV_LINK_WEIGHT_MAX - weight;

        mdv_router_node const *n1 = mdv_hashmap_find(topology.unique_nodes, link->id[0]);
        mdv_router_node const *n2 = mdv_hashmap_find(topology.unique_nodes, link->id[1]);

        mst_links[i].src = mst_nodes + n1->idx;
        mst_links[i].dst = mst_nodes + n2->idx;
    }

    size_t mst_size = mdv_mst_find(mst_nodes, mdv_hashmap_size(topology.unique_nodes),
                                   mst_links, mdv_vector_size(topology.links));

    if (mst_size)
    {
        mdv_vector *routes = mdv_vector_create(mst_size, sizeof(uint32_t), &mdv_default_allocator);

        if (!routes)
        {
            MDV_LOGE("No memory for routes");
            mdv_rollback(rollbacker);
            return 0;
        }

        for(size_t i = 0; i < mdv_vector_size(topology.links); ++i)
        {
            if (mst_links[i].mst)
            {
                uint32_t const src_node_id = (uint32_t)(size_t)mst_links[i].src->data;
                uint32_t const dst_node_id = (uint32_t)(size_t)mst_links[i].dst->data;

                if (src_node_id == MDV_LOCAL_ID)
                    mdv_vector_push_back(routes, &dst_node_id);
                else if (dst_node_id == MDV_LOCAL_ID)
                    mdv_vector_push_back(routes, &src_node_id);
            }
        }

        mdv_rollback(rollbacker);

        return routes;
    }

    mdv_rollback(rollbacker);

    return mdv_empty_vector;
}
