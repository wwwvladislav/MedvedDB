#include "mdv_mst.h"
#include "mdv_alloc.h"
#include "mdv_log.h"


typedef struct mdv_mstset mdv_mstset;

struct mdv_mstset
{
    size_t              set_id;     ///< MST subset identifier
    mdv_mstlink const  *link;       ///< link with minimum distance
    mdv_mstset         *next;       ///< Next subset pointer
};


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


static int mdv_mstlink_cmp(mdv_mstlink const *a, mdv_mstlink const *b)
{
    // todo
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
            subsets[i].link = 0;

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
            mdv_mstlink const *src_link = subsets[src_set_id].link;
            mdv_mstlink const *dst_link = subsets[dst_set_id].link;

            // Minimize the shortests link identifier for src set
            if (!src_link
                || src_link->weight > link->weight)
                subsets[src_set_id].link = link;

            // Minimize the shortests link identifier for dst set
            if (!dst_link
                || dst_link->weight > link->weight)
                subsets[dst_set_id].link = link;
        }

        for(size_t i = 0; i < nodes_count; ++i)
        {
            if (subsets[i].link)
            {
                // Get linked nodes identifiers
                size_t const src_id = subsets[i].link->src - nodes;
                size_t const dst_id = subsets[i].link->dst - nodes;

                // Get set identifiers for nodes
                size_t const src_set_id = subsets[src_id].set_id;
                size_t const dst_set_id = subsets[dst_id].set_id;

                if(src_set_id == dst_set_id)
                    continue;       // link is from the same subset

                printf("MST edge: %zu - %zu\n", src_id, dst_id);

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
