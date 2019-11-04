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
#include "mdv_vector.h"
#include "mdv_hashmap.h"


/// Node description
typedef struct
{
    mdv_uuid    uuid;                   ///< Unique identifier
    char const *addr;                   ///< Address
} mdv_toponode;


/// Link description
typedef struct
{
    uint32_t    node[2];                ///< Linked nodes indices
    uint32_t    weight;                 ///< Link weight. Bigger is better.
} mdv_topolink;


/// Topology description
typedef struct mdv_topology mdv_topology;


extern mdv_topology mdv_empty_topology;


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
 * @brief Creates new network topology
 *
 * @param nodes [in]        Nodes array (vector<mdv_toponode>)
 * @param links [in]        Links array (vector<mdv_topolink>)
 * @param extradata [in]    Additional data
 *
 * @return if there is no errors returns non-zero topology pointer
 */
mdv_topology * mdv_topology_create(mdv_vector *nodes, mdv_vector *links, mdv_vector *extradata);


/**
 * @brief Retains topology.
 * @details Reference counter is increased by one.
 */
mdv_topology * mdv_topology_retain(mdv_topology *topology);


/**
 * @brief Releases topology.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the topology's destructor is called.
 */
uint32_t mdv_topology_release(mdv_topology *topology);


/**
 * @brief Returns topology nodes
 */
mdv_vector * mdv_topology_nodes(mdv_topology *topology);


/**
 * @brief Returns topology links
 */
mdv_vector * mdv_topology_links(mdv_topology *topology);


/**
 * @brief Returns topology extradata vector
 */
mdv_vector * mdv_topology_extradata(mdv_topology *topology);


/**
 * @brief Topologies difference calculation (i.e. {a} - {b})
 * @details Links should be sorted in ascending order.
 *
 * @param a [in]        first topology
 * @param nodea [in]    uuid node where the first topology was obtained
 * @param b [in]        second topology
 * @param nodeb [in]    uuid node where the second topology was obtained
 *
 * @return topologies difference
 */
mdv_topology * mdv_topology_diff(mdv_topology *a, mdv_uuid const *nodea,
                                 mdv_topology *b, mdv_uuid const *nodeb);


/**
 * @brief Returns neighbours UUIDs for given node.
 *
 * @param topology [in] network topology
 * @param node [in]     node uuid
 *
 * @return neighbours UUIDs for given node.
 */
mdv_hashmap * mdv_topology_peers(mdv_topology *topology, mdv_uuid const *node);

