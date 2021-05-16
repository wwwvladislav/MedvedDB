#include "mdv_mst.h"
#include "mdv_alloc.h"
#include <mdv_log.h>


typedef struct mdv_mstset mdv_mstset;

struct mdv_mstset
{
    size_t       set_id;    ///< MST subset identifier
    mdv_mstlink *link;      ///< link with minimum distance
    mdv_mstset  *next;      ///< Next subset pointer
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
    int w = a->weight - b->weight;
    if (w)
        return w;

    mdv_mstnode const *a1, *a2;

    if (a->src < a->dst)
    {
        a1 = a->src;
        a2 = a->dst;
    }
    else
    {
        a1 = a->dst;
        a2 = a->src;
    }

    mdv_mstnode const *b1, *b2;

    if (b->src < b->dst)
    {
        b1 = b->src;
        b2 = b->dst;
    }
    else
    {
        b1 = b->dst;
        b2 = b->src;
    }

    w = a1 - b1;
    if (w)
        return w;

    return a2 - b2;
}


size_t mdv_mst_find(mdv_mstnode const *nodes, size_t nodes_count,
                    mdv_mstlink       *links, size_t links_count)
{
    mdv_mstset *subsets = mdv_alloc(sizeof(mdv_mstset) * nodes_count);

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

    for(size_t i = 0; i < links_count; ++i)
        links[i].mst = 0;

    size_t num_trees = nodes_count;
    size_t mst_size = 0;

    while (num_trees > 1)
    {
        // initialize subsets links
        for(size_t i = 0; i < nodes_count; ++i)
            subsets[i].link = 0;

        // search the closest subsets
        for(size_t i = 0; i < links_count; ++i)
        {
            mdv_mstlink *link = links + i;

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
                || mdv_mstlink_cmp(src_link, link) > 0)
                subsets[src_set_id].link = link;

            // Minimize the shortests link identifier for dst set
            if (!dst_link
                || mdv_mstlink_cmp(dst_link, link) > 0)
                subsets[dst_set_id].link = link;
        }

        size_t num_trees_prev = num_trees;

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

                subsets[i].link->mst = 1;
                mst_size++;

                // Union subsets
                mdv_union_subsets(subsets, src_set_id, dst_set_id);

                --num_trees;
            }
        }

        if (num_trees_prev == num_trees)    // some network segments is isolated
        {
            mst_size = 0;
            break;
        }
    }

    mdv_free(subsets);

    return mst_size;
}
