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


typedef struct mdv_mstnode
{
    void                *data;
    uint32_t             weight;
//    struct mdv_mst_node *
} mdv_mstnode;


typedef struct
{

} mdv_mst;
