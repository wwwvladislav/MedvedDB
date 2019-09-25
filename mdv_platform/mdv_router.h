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
#include "mdv_tracker.h"
#include "mdv_vector.h"


/**
 * @brief Find best routes
 *
 * @param routes [out]  routes
 * @param tracker [in]  Network topology tracker
 *
 * @return Vector of peers identifiers.
 */
mdv_vector * mdv_routes_find(mdv_tracker *tracker);
