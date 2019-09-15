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


/// Neighbor node identifiers for data broadcasting
typedef mdv_vector(uint32_t) mdv_routes;


/// Router for data synchronization using optimal routes
typedef struct
{
    mdv_routes routes;      ///< Best routes
} mdv_router;


/**
 * @brief Create router
 *
 * @param router [out] router for initialization
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_router_create(mdv_router *router);


/**
 * @brief Free router created by mdv_router_create()
 *
 * @param router [in] router for freeing
 */
void mdv_router_free(mdv_router *router);


/**
 * @brief Update best routes vector
 *
 * @param router [in]   router
 * @param tracker [in]  Network topology tracker
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_router_update(mdv_router *router, mdv_tracker *tracker);
