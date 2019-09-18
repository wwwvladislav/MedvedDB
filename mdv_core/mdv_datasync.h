/**
 * @file mdv_datasync.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Data synchronization module
 * @version 0.1
 * @date 2019-09-16
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_router.h>
#include <mdv_tracker.h>
#include <mdv_mutex.h>
#include <mdv_threadpool.h>
#include "storage/mdv_tablespace.h"


/// Data synchronizer configuration
typedef struct
{
    mdv_threadpool_config   threadpool;     ///< Thread pool options
    mdv_tablespace         *tablespace;     ///< DB tables space
} mdv_datasync_config;


/// Data synchronizer
typedef struct
{
    mdv_mutex       mutex;          ///< Mutex for routes guard
    mdv_routes      routes;         ///< Routes for data synchronization
    mdv_tablespace *tablespace;     ///< DB tables space
    mdv_threadpool *threadpool;     ///< Thread pool for data synchronization
} mdv_datasync;


/**
 * @brief Create data synchronizer
 *
 * @param datasync [out]    Data synchronizer for initialization
 * @param config [in]       Data synchronizer configuration
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_datasync_create(mdv_datasync *datasync, mdv_datasync_config const *config);


/**
 * @brief Free data synchronizer created by mdv_datasync_create()
 *
 * @param datasync [in] Data synchronizer for freeing
 */
void mdv_datasync_free(mdv_datasync *datasync);


/**
 * @brief Update data synchronization routes
 *
 * @param datasync [in] Data synchronizer
 * @param tracker [in]  Nodes and network topology tracker
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_datasync_update_routes(mdv_datasync *datasync, mdv_tracker *tracker);


/**
 * @brief Synchronize transaction logs
 *
 * @param datasync [in] Data synchronizer
 *
 * @return true if data synchronization is required
 * @return false if data synchronization is completed
 */
bool mdv_datasync_do(mdv_datasync *datasync);
