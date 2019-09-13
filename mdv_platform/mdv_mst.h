/**
 * @file mdv_mst.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Minimum Spanning Tree calculation algorithm implementation.
 * @details Boruvka/Sollin's algorithm is used for finding the MST.
 * @version 0.1
 * @date 2019-09-11
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 */
#pragma once
#include "mdv_def.h"


/// Graph node
typedef struct
{
    void *data;     ///< Data associated with node
} mdv_mstnode;



/// Link between two graph nodes
typedef struct mdv_mstlink
{
    mdv_mstnode const *src;     ///< Link source
    mdv_mstnode const *dst;     ///< Link destination
    uint32_t           weight;  ///< Link weight
    uint32_t           mst;     ///< non-zero value if link belongs to MST
} mdv_mstlink;


/**
 * @brief Find all MST links.
 * @details The 'mst' flag is initialized with non-zero value for all MST links.
 *
 * @param nodes [in]        Graph nodes
 * @param nodes_count [in]  Graph nodes count
 * @param links [in] [out]  Graph links
 * @param links_count [in]  Graph links count
 *
 * @return MST links count
 */
size_t mdv_mst_find(mdv_mstnode const *nodes, size_t nodes_count,
                    mdv_mstlink       *links, size_t links_count);
