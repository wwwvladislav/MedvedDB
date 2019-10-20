/**
 * @file mdv_graph.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Graph algorithms implementation
 * @version 0.1
 * @date 2019-10-20
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_def.h"


/// Link description
typedef struct
{
    uint32_t    node[2];                ///< Linked nodes indices
    uint32_t    weight;                 ///< Link weight. Bigger is better.
} mdv_glink;

