/**
 * @file mdv_evt_topology.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Topology events definitions
 * @version 0.1
 * @date 2019-10-22
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_ebus.h>
#include <mdv_topology.h>
#include <mdv_hashmap.h>


typedef struct
{
    mdv_event       base;
    mdv_topology   *topology;   ///< Network topology
} mdv_evt_topology;

mdv_evt_topology * mdv_evt_topology_create(mdv_topology *topology);
mdv_evt_topology * mdv_evt_topology_retain(mdv_evt_topology *evt);
uint32_t           mdv_evt_topology_release(mdv_evt_topology *evt);


typedef struct
{
    mdv_event       base;
    mdv_uuid        from;       ///< Node UUID from which the event was received
    mdv_uuid        to;         ///< Remote UUID for topology synchronization
    mdv_topology   *topology;   ///< Network topology
} mdv_evt_topology_sync;


mdv_evt_topology_sync * mdv_evt_topology_sync_create(mdv_uuid const *from,
                                                     mdv_uuid const *to,
                                                     mdv_topology   *topology);
mdv_evt_topology_sync * mdv_evt_topology_sync_retain(mdv_evt_topology_sync *evt);
uint32_t                mdv_evt_topology_sync_release(mdv_evt_topology_sync *evt);
