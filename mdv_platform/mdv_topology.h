/**
 * @file mdv_topology.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Network topology description and functions for extraction and serialization.
 * @version 0.1
 * @date 2019-08-26
 * @copyright Copyright (c) 2019, Vladislav Volkov
 */
#pragma once
#include "mdv_uuid.h"


/// Link description
typedef struct
{
    mdv_uuid   *node[2];                ///< Linked nodes identifiers
    uint32_t    weight;                 ///< Link weight. Bigger is better.
} mdv_link;


/// Topology description
typedef struct
{
    size_t       nodes_count;           ///< Nodes count
    size_t       links_count;           ///< Links count

    mdv_uuid    *nodes;                 ///< Nodes unique identifiers array
    mdv_link    *links;                 ///< Links array
} mdv_topology;


/// Nodes and topology tracker
struct mdv_tracker;


/**
 * @brief Extract network topology from tracker
 *
 * @param tracker [in]  Topology tracker
 *
 * @return On success return non NULL pointer to a network topology.
 */
mdv_topology * mdv_topology_extract(struct mdv_tracker *tracker);


/**
 * @brief Free the network topology
 *
 * @param topology [in] network topology
 */
void mdv_topology_free(mdv_topology *topology);

