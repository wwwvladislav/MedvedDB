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


typedef struct
{
    void *data;
} mdv_mstnode;


typedef struct mdv_mstlink
{
    mdv_mstnode const *src;
    mdv_mstnode const *dst;
    uint32_t           weight;
} mdv_mstlink;


typedef struct
{
    size_t      size;
    mdv_mstlink links[1];
} mdv_mst;


mdv_mst * mdv_mst_find(mdv_mstnode const *nodes, size_t nodes_count,
                       mdv_mstlink const *links, size_t links_count);
