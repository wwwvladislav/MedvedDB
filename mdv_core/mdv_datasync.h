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
#include <mdv_threads.h>
#include <mdv_eventfd.h>
#include <stdatomic.h>
#include "storage/mdv_tablespace.h"


/// Data synchronizer configuration
typedef struct
{
    mdv_tablespace         *tablespace;     ///< DB tables space
} mdv_datasync_config;


/// Data synchronizer
typedef struct
{
    mdv_mutex       mutex;          ///< Mutex for routes guard
    mdv_routes      routes;         ///< Routes for data synchronization
    mdv_tablespace *tablespace;     ///< DB tables space
    mdv_tracker    *tracker;        ///< Nodes and network topology tracker
    mdv_jobber     *jobber;         ///< Jobs scheduler
    mdv_descriptor  start;          ///< Signal for synchronization starting
    mdv_thread      thread;         ///< Synchronization thread
    atomic_bool     active;         ///< Data synchronizer status
} mdv_datasync;


/**
 * @brief Create data synchronizer
 *
 * @param datasync [out]    Data synchronizer for initialization
 * @param tablespace [in]   Storage
 * @param tracker [in]      Nodes and network topology tracker
 * @param jobber [in]       Jobs scheduler
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_datasync_create(mdv_datasync *datasync,
                              mdv_tablespace *tablespace,
                              mdv_tracker *tracker,
                              mdv_jobber *jobber);


/**
 * @brief Stop data synchronization
 *
 * @param datasync [in] Data synchronizer
 */
void mdv_datasync_stop(mdv_datasync *datasync);


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
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_datasync_update_routes(mdv_datasync *datasync);


/**
 * @brief Start transaction logs synchronization
 *
 * @param datasync [in] Data synchronizer
 *
 * @return true if data synchronization is required
 * @return false if data synchronization is completed
 */
void mdv_datasync_start(mdv_datasync *datasync);
