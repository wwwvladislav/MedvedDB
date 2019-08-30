#include "mdv_topology.h"
#include "mdv_tracker.h"
#include "mdv_vector.h"
#include "mdv_hashmap.h"
#include "mdv_rollbacker.h"
#include "mdv_log.h"
#include <stdlib.h>


static mdv_topology empty_topology =
{
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
    uint32_t id;
    uint32_t idx;
    mdv_uuid uuid;
} mdv_topology_node_id;


static size_t mdv_u32_hash(uint32_t const *v)
{
    return *v;
}


static int mdv_u32_keys_cmp(uint32_t const *a, uint32_t const *b)
{
    return (int)*a - *b;
}


typedef mdv_vector(mdv_uuid)            mdv_nodes_array;
typedef mdv_vector(mdv_tracker_link)    mdv_links_array;


typedef struct
{
    struct mdv_tracker *tracker;
    mdv_links_array     links;
    mdv_hashmap         unique_ids;
} mdv_tracker_links_tmp;


static mdv_topology_node_id const *mdv_topology_node_id_register(mdv_tracker_links_tmp *tmp, uint32_t id)
{
    mdv_topology_node_id const *nid = mdv_hashmap_find(tmp->unique_ids, id);

    if (!nid)
    {
        mdv_uuid uuid;

        if (!mdv_tracker_node_uuid(tmp->tracker, id, &uuid))
        {
            MDV_LOGE("Node with %u identifier wasn't registered", id);
            return 0;
        }

        mdv_topology_node_id const node_id =
        {
            .id = id,
            .uuid = uuid
        };

        mdv_list_entry_base *entry = mdv_hashmap_insert(tmp->unique_ids, node_id);

        if (!entry)
        {
            MDV_LOGE("No memory for node id");
            return 0;
        }

        nid = (void*)entry->data;
    }

    return nid;
}


static void mdv_tracker_link_add(mdv_tracker_link const *link, void *arg)
{
    mdv_tracker_links_tmp *tmp = arg;

    if (mdv_vector_push_back(tmp->links, *link))
    {
        mdv_topology_node_id const *id0 = mdv_topology_node_id_register(tmp, link->id[0]);
        mdv_topology_node_id const *id1 = mdv_topology_node_id_register(tmp, link->id[1]);

        if (id0 && id1)
            return;
    }

    MDV_LOGE("No memory for network topology link");
}


int mdv_link_cmp(mdv_link const *a, mdv_link const *b)
{
    if (mdv_uuid_cmp(a->node[0], b->node[0]) < 0)
        return -1;
    else if (mdv_uuid_cmp(a->node[0], b->node[0]) > 0)
        return 1;
    else if (mdv_uuid_cmp(a->node[1], b->node[1]) < 0)
        return -1;
    else if (mdv_uuid_cmp(a->node[1], b->node[1]) > 0)
        return 1;
    return 0;
}


static size_t mdv_topology_size(size_t nodes_count, size_t links_count)
{
    return sizeof(mdv_topology)
            + sizeof(mdv_uuid) * nodes_count
            + sizeof(mdv_link) * links_count;
}


mdv_topology * mdv_topology_extract(struct mdv_tracker *tracker)
{
    size_t const links_count = mdv_tracker_links_count(tracker);

    if (!links_count)
        return &empty_topology;

    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_tracker_links_tmp tmp =
    {
        .tracker = tracker
    };

    if (!mdv_vector_create(tmp.links, links_count + 8, mdv_stallocator))
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_stfree, tmp.links.data, "tmp.links");

    if (!mdv_hashmap_init(tmp.unique_ids, mdv_topology_node_id, id, 64, mdv_u32_hash, mdv_u32_keys_cmp))
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &tmp.unique_ids);

    mdv_tracker_links_foreach(tracker, &tmp, &mdv_tracker_link_add);

    if (mdv_vector_empty(tmp.links))
    {
        mdv_rollback(rollbacker);
        return &empty_topology;
    }

    mdv_topology *topology = mdv_alloc(
                                mdv_topology_size(
                                    mdv_hashmap_size(tmp.unique_ids),
                                    mdv_vector_size(tmp.links)), "topology");

    if (!topology)
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, topology, "topology");

    topology->nodes_count = mdv_hashmap_size(tmp.unique_ids);
    topology->links_count = mdv_vector_size(tmp.links);
    topology->nodes       = (void*)(topology + 1);
    topology->links       = (void*)(topology->nodes + topology->nodes_count);

    uint32_t n = 0;

    mdv_hashmap_foreach(tmp.unique_ids, mdv_topology_node_id, entry)
    {
        entry->idx = n;
        topology->nodes[n] = entry->uuid;
        ++n;
    }

    n = 0;

    mdv_vector_foreach(tmp.links, mdv_tracker_link, link)
    {
        mdv_topology_node_id const *id0 = mdv_hashmap_find(tmp.unique_ids, link->id[0]);
        mdv_topology_node_id const *id1 = mdv_hashmap_find(tmp.unique_ids, link->id[1]);

        if (!id0 || !id1)
        {
            MDV_LOGE("Invalid network topology");
            mdv_rollback(rollbacker);
            return 0;
        }

        mdv_link *l = topology->links + n;

        l->node[0] = topology->nodes + id0->idx;
        l->node[1] = topology->nodes + id1->idx;
        l->weight = link->weight;

        ++n;
    }

    mdv_hashmap_free(tmp.unique_ids);
    mdv_vector_free(tmp.links);

    qsort(topology->links,
          topology->links_count,
          sizeof *topology->links,
          (int (*)(const void *, const void *))&mdv_link_cmp);

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
    else if (!a->links_count)
    {
//        mdv_topology_delta *delta = mdv_alloc(
//                                        sizeof(mdv_topology_delta) +
//                                        mdv_topology_size(), "topology_delta");
    }
    else if (!b->links_count)
    {
    }

    // TODO:
    return &empty_topology_delta;
}


void mdv_topology_delta_free(mdv_topology_delta *delta)
{}
