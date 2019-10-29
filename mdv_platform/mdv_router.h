/**
 * @file mdv_router.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Routes calculator for data synchronization.
 * @version 0.1
 * @date 2019-09-06
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 */
#pragma once
#include "mdv_topology.h"
#include "mdv_hashmap.h"


/// Next hop information
typedef struct
{
    mdv_uuid uuid;  ///< Next hop uuid
} mdv_route;


/**
 * @brief Find best routes for specified source node
 *
 * @param topology [in] Network topology
 * @param src [in]      Source node uuid (is used for next hop searching)
 *
 * @return Vector of peers identifiers (vector<uuid>).
 */
mdv_hashmap * mdv_routes_find(mdv_topology *topology, mdv_uuid const *src);
