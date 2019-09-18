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
#include <mdv_jobber.h>
#include "storage/mdv_tablespace.h"
#include <stdatomic.h>


/// Data synchronizer configuration
typedef struct
{
    mdv_tablespace         *tablespace;     ///< DB tables space
} mdv_datasync_config;


/**
 * @brief Data synchronizer statuses
 * @details Valid states:
        SCHEDULED
        STARTED
        STARTED + RESTART
 */
typedef enum
{
    MDV_DS_SCHEDULED    = 1 << 0,
    MDV_DS_STARTED      = 1 << 1,
    MDV_DS_RESTART      = 1 << 2
} mdv_datasync_status;


/// Data synchronizer
typedef struct
{
    mdv_mutex       mutex;          ///< Mutex for routes guard
    mdv_routes      routes;         ///< Routes for data synchronization
    mdv_tablespace *tablespace;     ///< DB tables space
    atomic_uint     status;         ///< Synchronization status
} mdv_datasync;


/**
 * @brief Create data synchronizer
 *
 * @param datasync [out]    Data synchronizer for initialization
 * @param tablespace [in]   Storage
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_datasync_create(mdv_datasync *datasync, mdv_tablespace *tablespace);


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
 * @brief Start transaction logs synchronization
 *
 * @param datasync [in] Data synchronizer
 * @param tracker [in]  Nodes and network topology tracker
 * @param jobber [in]   Jobs scheduler
 *
 * @return true if data synchronization is required
 * @return false if data synchronization is completed
 */
void mdv_datasync_start(mdv_datasync *datasync, mdv_tracker *tracker, mdv_jobber *jobber);
