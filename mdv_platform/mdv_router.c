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
} mdv_router_node;


static size_t mdv_uint32_hash(void const *p)                    { return *(uint32_t*)p; }
static int    mdv_uint32_cmp(void const *pa, void const *pb)    { return *(uint32_t*)pa - *(uint32_t*)pb; }


static mdv_hashmap * mdv_router_unique_nodes(mdv_vector *topolinks, mdv_vector *toponodes)
{
    mdv_hashmap *nodes = mdv_hashmap_create(mdv_router_node,
                                            id,
                                            mdv_vector_size(toponodes) * 5 / 4,
                                            mdv_uint32_hash,
                                            mdv_uint32_cmp);

    if (nodes)
    {
        mdv_vector_foreach(topolinks, mdv_topolink, link)
        {
            if (!mdv_hashmap_find(nodes, link->node + 0))
            {
                mdv_router_node n = { .id = link->node[0] };
                mdv_hashmap_insert(nodes, &n, sizeof n);
            }

            if (!mdv_hashmap_find(nodes, link->node + 1))
            {
                mdv_router_node n = { .id = link->node[1] };
                mdv_hashmap_insert(nodes, &n, sizeof n);
            }
        }

        uint32_t idx = 0;

        mdv_hashmap_foreach(nodes, mdv_router_node, node)
            node->idx = idx++;
    }

    return nodes;
}


mdv_hashmap * mdv_routes_find(mdv_topology *topology, mdv_uuid const *src)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(5);

    mdv_vector *topolinks = mdv_topology_links(topology);

    mdv_rollbacker_push(rollbacker, mdv_vector_release, topolinks);

    if (mdv_vector_empty(topolinks))
    {
        mdv_rollback(rollbacker);
        return mdv_hashmap_create(mdv_route,
                                  uuid,
                                  0,
                                  mdv_uuid_hash,
                                  mdv_uuid_cmp);
    }

    mdv_vector *toponodes = mdv_topology_nodes(topology);

    mdv_rollbacker_push(rollbacker, mdv_vector_release, toponodes);

    mdv_hashmap *unique_nodes = mdv_router_unique_nodes(topolinks, toponodes);

    if (!unique_nodes)
    {
        MDV_LOGE("No memorty for MST nodes");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, unique_nodes);

    mdv_mstnode *mst_nodes = mdv_alloc(mdv_vector_size(toponodes) * sizeof(mdv_mstnode), "mst_nodes");

    if (!mst_nodes)
    {
        MDV_LOGE("No memorty for MST nodes");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, mst_nodes, "mst_nodes");

    mdv_hashmap_foreach(unique_nodes, mdv_router_node, node)
        mst_nodes[node->idx].data = mdv_vector_at(toponodes, node->id);

    mdv_mstlink *mst_links = mdv_alloc(mdv_vector_size(topolinks) * sizeof(mdv_mstlink), "mst_links");

    if (!mst_links)
    {
        MDV_LOGE("No memorty for MST links");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, mst_links, "mst_links");

    size_t i = 0;

    mdv_vector_foreach(topolinks, mdv_topolink, link)
    {
        mdv_mstlink *mstlink = mst_links + i++;

        uint32_t weight = link->weight > MDV_LINK_WEIGHT_MAX
                            ? MDV_LINK_WEIGHT_MAX:
                            link->weight;

        mdv_router_node const *n0 = mdv_hashmap_find(unique_nodes, link->node + 0);
        mdv_router_node const *n1 = mdv_hashmap_find(unique_nodes, link->node + 1);

        mstlink->src = mst_nodes + n0->idx;
        mstlink->dst = mst_nodes + n1->idx;
        mstlink->weight = MDV_LINK_WEIGHT_MAX - weight;
    }

    size_t mst_size = mdv_mst_find(mst_nodes, mdv_hashmap_size(unique_nodes),
                                   mst_links, mdv_vector_size(topolinks));

    if (mst_size)
    {
        mdv_hashmap *routes = mdv_hashmap_create(
                                        mdv_route,
                                        uuid,
                                        mst_size * 5 / 3,
                                        mdv_uuid_hash,
                                        mdv_uuid_cmp);

        if (!routes)
        {
            MDV_LOGE("No memory for routes");
            mdv_rollback(rollbacker);
            return 0;
        }

        for(size_t i = 0; i < mdv_vector_size(topolinks); ++i)
        {
            if (mst_links[i].mst)
            {
                mdv_toponode const *lnode = mst_links[i].src->data;
                mdv_toponode const *rnode = mst_links[i].dst->data;

                if (mdv_uuid_cmp(src, &lnode->uuid) == 0)
                {
                    mdv_route const route = { rnode->uuid };

                    if (!mdv_hashmap_insert(routes, &route, sizeof route))
                    {
                        mdv_hashmap_release(routes);
                        MDV_LOGE("No memory for routes");
                        mdv_rollback(rollbacker);
                        return 0;
                    }
                }
                else if (mdv_uuid_cmp(src, &rnode->uuid) == 0)
                {
                    mdv_route const route = { lnode->uuid };

                    if (!mdv_hashmap_insert(routes, &route, sizeof route))
                    {
                        mdv_hashmap_release(routes);
                        MDV_LOGE("No memory for routes");
                        mdv_rollback(rollbacker);
                        return 0;
                    }
                }
            }
        }

        mdv_rollback(rollbacker);

        return routes;
    }

    mdv_rollback(rollbacker);

    return mdv_hashmap_create(mdv_route,
                                uuid,
                                0,
                                mdv_uuid_hash,
                                mdv_uuid_cmp);
}
