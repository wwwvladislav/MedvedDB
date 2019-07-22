/**
 * @file
 * @brief Nodes and topology tracker
 */

#pragma once
#include "storage/mdv_nodes.h"


/// Nodes and topology tracker
typedef struct mdv_tracker
{
    mdv_nodes *nodes;       ///< nodes storage
} mdv_tracker;


/**
 * @brief Create new topology tracker
 *
 * @param tracker [in] [out]    Topology tracker to be initialized
 * @param nodes [in]            Nodes storage
 */
void mdv_tracker_create(mdv_tracker *tracker, mdv_nodes *nodes);


/**
 * @brief   Free a topology tracker
 *
 * @param tracker [in] Topology tracker
 */
void mdv_tracker_free(mdv_tracker *tracker);


/**
 * @brief Register new node in storage
 *
 * @param tracker [in]      Topology tracker
 * @param node [in] [out]   node information
 *
 * @return On success, return MDV_OK and node->id is initialized by local unique numeric identifier
 * @return On error, return nonzero error code
 */
mdv_errno mdv_tracker_reg(mdv_tracker *tracker, mdv_node *node);


/**
 * @brief Unregister node in storage.
 * @details Node is not deleted but only is marked as inactive.
 *
 * @param tracker [in]  Topology tracker
 * @param uuid [in]     node UUID
 */
void mdv_tracker_unreg(mdv_tracker *tracker, mdv_uuid const *uuid);
