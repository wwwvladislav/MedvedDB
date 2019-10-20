/**
 * @file
 * @brief Storage for cluster nodes.
 * @details Nodes storage contains information about nodes (identifiers, UUIDs and addresses).
*/
#pragma once
#include "mdv_storage.h"
#include "../mdv_node.h"


/**
 * @brief Iterates over all nodes and call function fn.
 *
 * @param storage [in]  storage where nodes saved
 * @param tracker [out] nodes tracker
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero value
 */
mdv_errno mdv_nodes_foreach(mdv_storage *storage,
                            void *arg,
                            void (*fn)(void *arg, mdv_node const *node));


/**
 * @brief Store new node in storage
 *
 * @param storage [in]  storage where nodes saved
 * @param node [in]     node information
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero value
 */
mdv_errno mdv_nodes_store(mdv_storage *storage, mdv_node const *node);


/**
 * @brief Returns current node description
 *
 * @param uuid [in] node uuid
 * @param addr [in] node address
 *
 * @return current node description
 */
mdv_node const * mdv_nodes_current(mdv_uuid const *uuid, char const *addr);
