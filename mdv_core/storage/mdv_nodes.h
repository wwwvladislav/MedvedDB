/**
 * @file
 * @brief Storage for cluster nodes and topology.
 * @details Nodes storage contains information about nodes (identifiers, addresses) and connectivity state between nodes.
*/
#pragma once
#include "mdv_storage.h"
#include <mdv_uuid.h>
#include <mdv_hashmap.h>
#include <mdv_def.h>
#include <mdv_mutex.h>


/// Cluster node information
typedef struct
{
    size_t      size;       ///< Current data structure size
    mdv_uuid    uuid;       ///< Global unique identifier
    uint32_t    id;         ///< Unique identifier inside current server
    uint8_t     active:1;   ///< Node is active. Connection establisched.
    char        addr[1];    ///< Node address in following format: protocol://host:port
} mdv_node;


/// Nodes storage descriptor
typedef struct
{
    volatile uint32_t    max_id;        ///< Maximum node identifier

    mdv_hashmap          nodes;         ///< Nodes map (UUID -> mdv_node)
    mdv_mutex            nodes_mutex;   ///< nodes guard mutex

    mdv_hashmap          ids;           ///< Node identifiers (id -> UUID)
    mdv_mutex            ids_mutex;     ///< node identifiers guard mutex

    mdv_storage         *storage;       ///< Key-value storage for nodes
} mdv_nodes;


/**
 * @brief Load nodes from storage
 *
 * @param nodes [out] nodes in-memory storage
 * @param storage [in] storage where nodes saved
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero value
 */
mdv_errno mdv_nodes_load(mdv_nodes *nodes, mdv_storage *storage);


/**
 * @brief Free nodes storage
 *
 * @param nodes [in] nodes in-memory storage
 */
void mdv_nodes_free(mdv_nodes *nodes);


/**
 * @brief Register new node in storage
 *
 * @param nodes [in] nodes in-memory storage
 * @param node [in] [out] node information
 *
 * @return On success, return MDV_OK and node->id is initialized by local unique numeric identifier
 * @return On error, return nonzero value
 */
mdv_errno mdv_nodes_reg(mdv_nodes *nodes, mdv_node *node);
