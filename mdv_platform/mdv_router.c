#include "mdv_router.h"
#include "mdv_rollbacker.h"
#include "mdv_mst.h"
#include "mdv_log.h"
#include "mdv_limits.h"
#include <stdlib.h>


mdv_vector * mdv_routes_find(mdv_topology *topology, mdv_uuid const *src)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    mdv_vector *topolinks = mdv_topology_links(topology);

    mdv_rollbacker_push(rollbacker, mdv_vector_release, topolinks);

    if (mdv_vector_empty(topolinks))
    {
        mdv_rollback(rollbacker);
        return &mdv_empty_vector;
    }

    mdv_vector *toponodes = mdv_topology_nodes(topology);

    mdv_rollbacker_push(rollbacker, mdv_vector_release, toponodes);

    mdv_mstnode *mst_nodes = mdv_stalloc(mdv_vector_size(toponodes) * sizeof(mdv_mstnode), "mst_nodes");

    if (!mst_nodes)
    {
        MDV_LOGE("No memorty for MST nodes");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_stfree, mst_nodes, "mst_nodes");

    size_t i = 0;

    mdv_vector_foreach(toponodes, mdv_toponode, node)
    {
        mst_nodes[i++].data = node;
    }

    mdv_mstlink *mst_links = mdv_stalloc(mdv_vector_size(topolinks) * sizeof(mdv_mstlink), "mst_links");

    if (!mst_links)
    {
        MDV_LOGE("No memorty for MST links");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_stfree, mst_links, "mst_links");

    i = 0;

    mdv_vector_foreach(topolinks, mdv_topolink, link)
    {
        mdv_mstlink *mstlink = mst_links + i++;

        uint32_t weight = link->weight > MDV_LINK_WEIGHT_MAX
                            ? MDV_LINK_WEIGHT_MAX:
                            link->weight;

        mstlink->src = mst_nodes + link->node[0];
        mstlink->dst = mst_nodes + link->node[1];
        mstlink->weight = MDV_LINK_WEIGHT_MAX - weight;
    }

    size_t mst_size = mdv_mst_find(mst_nodes, mdv_vector_size(toponodes),
                                   mst_links, mdv_vector_size(topolinks));

    if (mst_size)
    {
        mdv_vector *routes = mdv_vector_create(mst_size, sizeof(mdv_uuid), &mdv_default_allocator);

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
                    mdv_vector_push_back(routes, &rnode->uuid);
                else if (mdv_uuid_cmp(src, &rnode->uuid) == 0)
                    mdv_vector_push_back(routes, &lnode->uuid);
            }
        }

        mdv_rollback(rollbacker);

        return routes;
    }

    mdv_rollback(rollbacker);

    return &mdv_empty_vector;
}
