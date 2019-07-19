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


mdv_errno mdv_tracker_register(mdv_tracker *tracker, mdv_node *node);

