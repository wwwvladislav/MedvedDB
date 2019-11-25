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
#include <mdv_ebus.h>
#include <mdv_jobber.h>
#include <mdv_topology.h>
#include <mdv_uuid.h>


/// Data synchronizer
typedef struct mdv_datasync mdv_datasync;


/**
 * @brief Creates data synchronizer
 *
 * @param uuid [in]         Global unique identifier for current node
 * @param ebus [in]         Events bus
 * @param jconfig [in]      Jobs scheduler configuration
 * @param topology [in]     Network topology
 *
 * @return Data committer
 */
mdv_datasync * mdv_datasync_create(mdv_uuid const *uuid,
                                   mdv_ebus *ebus,
                                   mdv_jobber_config const *jconfig,
                                   mdv_topology *topology);


/**
 * @brief Retains data synchronizer.
 * @details Reference counter is increased by one.
 */
mdv_datasync * mdv_datasync_retain(mdv_datasync *datasync);


/**
 * @brief Releases data synchronizer.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the data synchronizer is stopped and freed.
 */
uint32_t mdv_datasync_release(mdv_datasync *datasync);


/**
 * @brief Start transaction logs synchronization
 *
 * @param datasync [in] Data synchronizer
 *
 * @return true if data synchronization is required
 * @return false if data synchronization is completed
 */
void mdv_datasync_start(mdv_datasync *datasync);


/**
 * @brief Stop data synchronization
 *
 * @param datasync [in] Data synchronizer
 */
void mdv_datasync_stop(mdv_datasync *datasync);
