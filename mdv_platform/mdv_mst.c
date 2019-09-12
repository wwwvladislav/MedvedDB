#include "mdv_mst.h"
#include "mdv_alloc.h"
#include "mdv_log.h"


typedef struct mdv_mstset
{
    size_t              set_id;     ///< MST subset identifier
    size_t              link_id;    ///< link with minimum distance
    struct mdv_mstset   *next;      ///< Next subset pointer
} mdv_mstset;


static void mdv_union_subsets(mdv_mstset *subsests, size_t id1, size_t id2)
{
    mdv_mstset *subset1_next = subsests[id1].next;

    subsests[id1].next = &subsests[id2];

    mdv_mstset *subset2_end = 0;

    for(mdv_mstset *p = &subsests[id2]; p && p->set_id != id1; p = p->next)
    {
        p->set_id = id1;
        subset2_end = p;
    }

    subset2_end->next = subset1_next ? subset1_next : &subsests[id1];
}


mdv_mst * mdv_mst_find(mdv_mstnode const *nodes, size_t nodes_count,
                       mdv_mstlink const *links, size_t links_count)
{
    mdv_mstset *subsets = mdv_staligned_alloc(sizeof(mdv_mstset), sizeof(mdv_mstset) * nodes_count, "MST subsets");

    if (!subsets)
    {
        MDV_LOGE("No memory for MST subsets");
        return 0;
    }

    for(size_t i = 0; i < nodes_count; ++i)
    {
        subsets[i].set_id = i;
        subsets[i].next = 0;
    }

    size_t num_trees = nodes_count;

    while (num_trees > 1)
    {
        // initialize subsets links
        for(size_t i = 0; i < nodes_count; ++i)
            subsets[i].link_id = ~0u;

        // search the closest subsets
        for(size_t i = 0; i < links_count; ++i)
        {
            mdv_mstlink const *link = links + i;

            // Get linked nodes identifiers
            size_t const src_id = link->src - nodes;
            size_t const dst_id = link->dst - nodes;

            // Get set identifiers for nodes
            size_t const src_set_id = subsets[src_id].set_id;
            size_t const dst_set_id = subsets[dst_id].set_id;

            if(src_set_id == dst_set_id)
                continue;       // link is from the same subset

            // Get shortests links identifiers for src and dst sets
            size_t const src_link_id = subsets[src_set_id].link_id;
            size_t const dst_link_id = subsets[dst_set_id].link_id;

            // Minimize the shortests link identifier for src set
            if (src_link_id == ~0u
                || links[src_link_id].weight > link->weight)
                subsets[src_set_id].link_id = i;

            // Minimize the shortests link identifier for dst set
            if (dst_link_id == ~0u
                || links[dst_link_id].weight > link->weight)
                subsets[dst_set_id].link_id = i;
        }

        for(size_t i = 0; i < nodes_count; ++i)
        {
            if (subsets[i].link_id != ~0)
            {
                // Get linked nodes identifiers
                size_t const src_id = links[subsets[i].link_id].src - nodes;
                size_t const dst_id = links[subsets[i].link_id].dst - nodes;

                // Get set identifiers for nodes
                size_t const src_set_id = subsets[src_id].set_id;
                size_t const dst_set_id = subsets[dst_id].set_id;

                if(src_set_id == dst_set_id)
                    continue;       // link is from the same subset

                // Union subsets
                mdv_union_subsets(subsets, src_set_id, dst_set_id);

                --num_trees;
            }
        }
    }

    // TODO: return MST

    mdv_stfree(subsets, "MST subsets");

    return 0;
}
