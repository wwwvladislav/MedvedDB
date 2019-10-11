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


/// Node description
typedef struct
{
    mdv_uuid    uuid;                   ///< Unique identifier
    char const *addr;                   ///< Address
} mdv_toponode;


/// Link description
typedef struct
{
    mdv_toponode   *node[2];            ///< Linked nodes
    uint32_t        weight;             ///< Link weight. Bigger is better.
} mdv_topolink;


/// Topology description
typedef struct
{
    size_t          size;               ///< topology data structure size
    size_t          nodes_count;        ///< Nodes count
    size_t          links_count;        ///< Links count

    mdv_toponode    *nodes;             ///< Nodes array
    mdv_topolink    *links;             ///< Links array
} mdv_topology;


/// Topologies difference
typedef struct
{
    mdv_topology *ab;                   ///< {a} - {b}
    mdv_topology *ba;                   ///< {b} - {a}
} mdv_topology_delta;


extern mdv_topology empty_topology;
extern mdv_topology_delta empty_topology_delta;


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
int mdv_link_cmp(mdv_topolink const *a, mdv_topolink const *b);


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
