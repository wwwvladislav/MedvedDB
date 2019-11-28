/**
 * @file mdv_syncer.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Transaction logs synchronizer
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
typedef struct mdv_syncer mdv_syncer;


/**
 * @brief Creates data synchronizer
 *
 * @param uuid [in]         Global unique identifier for current node
 * @param ebus [in]         Events bus
 * @param jconfig [in]      Jobs scheduler configuration
 * @param topology [in]     Network topology
 *
 * @return Data synchronizer
 */
mdv_syncer * mdv_syncer_create(mdv_uuid const *uuid,
                               mdv_ebus *ebus,
                               mdv_jobber_config const *jconfig,
                               mdv_topology *topology);


/**
 * @brief Retains data synchronizer.
 * @details Reference counter is increased by one.
 */
mdv_syncer * mdv_syncer_retain(mdv_syncer *syncer);


/**
 * @brief Releases data synchronizer.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the data synchronizer is stopped and freed.
 */
uint32_t mdv_syncer_release(mdv_syncer *syncer);


/**
 * @brief Cancels data synchronization
 *
 * @param syncer [in] Data synchronizer
 */
void mdv_syncer_cancel(mdv_syncer *syncer);
