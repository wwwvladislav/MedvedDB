/**
 * @file
 * @brief Nodes and topology tracker
 */

#pragma once
#include "mdv_def.h"
#include "mdv_uuid.h"
#include "mdv_hashmap.h"
#include "mdv_mutex.h"


/// Cluster node information
typedef struct
{
    size_t      size;       ///< Current data structure size
    mdv_uuid    uuid;       ///< Global unique identifier
    uint32_t    id;         ///< Unique identifier inside current server
    uint8_t     active:1;   ///< Node is active. Connection establisched.
    char        addr[1];    ///< Node address in following format: protocol://host:port
} mdv_node;


/// Nodes and topology tracker
typedef struct mdv_tracker
{
    volatile uint32_t    max_id;        ///< Maximum node identifier

    mdv_hashmap          nodes;         ///< Nodes map (UUID -> mdv_node)
    mdv_mutex            nodes_mutex;   ///< nodes guard mutex

    mdv_hashmap          ids;           ///< Node identifiers (id -> UUID)
    mdv_mutex            ids_mutex;     ///< node identifiers guard mutex
} mdv_tracker;


/**
 * @brief Create new topology tracker
 *
 * @param tracker [in] [out]    Topology tracker to be initialized
 * @param nodes [in]            Nodes storage
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero error code
 */
mdv_errno mdv_tracker_create(mdv_tracker *tracker);


/**
 * @brief   Free a topology tracker
 *
 * @param tracker [in] Topology tracker
 */
void mdv_tracker_free(mdv_tracker *tracker);


/**
 * @brief Register new node in storage
 *
 * @param tracker [in]          Topology tracker
 * @param new_node [in] [out]   node information
 *
 * @return On success, return MDV_OK and node->id is initialized by local unique numeric identifier
 * @return On error, return nonzero error code
 */
mdv_errno mdv_tracker_reg(mdv_tracker *tracker, mdv_node *new_node);


/**
 * @brief node into the tracker
 *
 * @param tracker [in]          Topology tracker
 * @param node [in] [out]       node information
 */
void mdv_tracker_append(mdv_tracker *tracker, mdv_node const *node);


/**
 * @brief Unregister node in storage.
 * @details Node is not deleted but only is marked as inactive.
 *
 * @param tracker [in]  Topology tracker
 * @param uuid [in]     node UUID
 */
void mdv_tracker_unreg(mdv_tracker *tracker, mdv_uuid const *uuid);
