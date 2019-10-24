/**
 * @file mdv_topology.h
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


typedef struct
{
    mdv_event       base;
    mdv_topology   *topology;
} mdv_evt_topology_changed;

mdv_evt_topology_changed * mdv_evt_topology_changed_create(mdv_topology *topology);
mdv_evt_topology_changed * mdv_evt_topology_changed_retain(mdv_evt_topology_changed *evt);
uint32_t                   mdv_evt_topology_changed_release(mdv_evt_topology_changed *evt);


typedef struct
{
    mdv_event       base;
    mdv_uuid        dst;
    mdv_topology   *topology;
} mdv_evt_topology_sync;

mdv_evt_topology_sync * mdv_evt_topology_sync_create(mdv_topology *topology, mdv_uuid const *dst);
mdv_evt_topology_sync * mdv_evt_topology_sync_retain(mdv_evt_topology_sync *evt);
uint32_t                mdv_evt_topology_sync_release(mdv_evt_topology_sync *evt);
