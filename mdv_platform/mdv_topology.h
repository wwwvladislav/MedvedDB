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


/// Topologies difference
typedef struct
{
    mdv_topology *ab;                   ///< {a} - {b}
    mdv_topology *ba;                   ///< {b} - {a}
} mdv_topology_delta;


/// Nodes and topology tracker
struct mdv_tracker;


/**
 * @brief Two links comparision
 *
 * @param a [in]    first link
 * @param b [in]    second link
 *
 * @return an integer less than zero if a is less then b
 * @return zero if a is equal to b
 * @return an integer greater than zero if a is greater then b
 */
int mdv_link_cmp(mdv_link const *a, mdv_link const *b);


/**
 * @brief Extract network topology from tracker
 * @details In result topology links are sorted in ascending order.
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


/**
 * @brief Topologies difference calculation (i.e. {a} - {b} and {b} - {a})
 * @details Links should be sorted in ascending order.
 *
 * @param a [in] first topology
 * @param b [in] second topology
 *
 * @return topologies difference
 */
mdv_topology_delta * mdv_topology_diff(mdv_topology const *a, mdv_topology const *b);


/**
 * @brief Free the network topologies difference
 *
 * @param delta [in] topologies difference
 */
void mdv_topology_delta_free(mdv_topology_delta *delta);