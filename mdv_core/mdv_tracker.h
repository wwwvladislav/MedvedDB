/**
 * @file
 * @brief Nodes and network topology tracker
 */
#pragma once
#include "mdv_node.h"
#include "storage/mdv_storage.h"
#include <mdv_def.h>
#include <mdv_uuid.h>
#include <mdv_vector.h>
#include <mdv_ebus.h>
#include <mdv_topology.h>


/// Link between nodes
typedef struct
{
    uint32_t    id[2];                  ///< Unique node identifiers inside current server
    uint32_t    weight;                 ///< Link weight. Bigger is better.
} mdv_tracker_link;


/// Nodes and network topology tracker
typedef struct mdv_tracker mdv_tracker;


/**
 * @brief Creates new topology tracker
 *
 * @param uuid [in]     Global unique identifier for current node
 * @param storage [in]  Nodes storage
 * @param ebus [in]     Events bus
 *
 * @return On success, return non-null pointer to tracker
 * @return On error, return NULL
 */
mdv_tracker * mdv_tracker_create(mdv_uuid const *uuid,
                                 mdv_storage    *storage,
                                 mdv_ebus       *ebus);


/**
 * @brief Retains topology tracker.
 * @details Reference counter is increased by one.
 */
mdv_tracker * mdv_tracker_retain(mdv_tracker *tracker);


/**
 * @brief Releases topology tracker.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the topology tracker is freed.
 */
void mdv_tracker_release(mdv_tracker *tracker);


/**
 * @brief Returns UUID for current node
 */
mdv_uuid const * mdv_tracker_uuid(mdv_tracker *tracker);


/**
 * @brief Return links count in network topology.
 *
 * @param tracker [in]          Topology tracker
 *
 * @return links count
 */
size_t mdv_tracker_links_count(mdv_tracker *tracker);


/**
 * @brief Iterates over all topology links and call function fn.
 *
 * @param tracker [in]          Topology tracker
 * @param arg [in]              User defined data which is provided as second argument to the function fn
 * @param fn [in]               function pointer
 */
void mdv_tracker_links_foreach(mdv_tracker *tracker,
                               void *arg,
                               void (*fn)(mdv_tracker_link const *, void *));


/**
 * @brief Return links vector
 *
 * @param tracker [in]  Topology tracker
 *
 * @return links vector (vector<mdv_tracker_link>)
 */
mdv_vector * mdv_tracker_links(mdv_tracker *tracker);


/**
 * @brief Extract network topology from tracker
 * @details In result topology links are sorted in ascending order.
 *
 * @param tracker [in]          Topology tracker
 *
 * @return On success return non NULL pointer to a network topology.
 */
mdv_topology * mdv_tracker_topology(mdv_tracker *tracker);
