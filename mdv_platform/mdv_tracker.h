/**
 * @file
 * @brief Nodes and network topology tracker
 */

#pragma once
#include "mdv_def.h"
#include "mdv_uuid.h"
#include "mdv_hashmap.h"
#include "mdv_mutex.h"
#include "mdv_vector.h"
#include "mdv_topology.h"


enum
{
    MDV_LOCAL_ID = 0                    ///< Local identifier for current node
};


/// Cluster node information
typedef struct
{
    size_t      size;                   ///< Current data structure size
    mdv_uuid    uuid;                   ///< Global unique identifier
    void       *userdata;               ///< Userdata associated with node (for instance, connection context)
    uint32_t    id;                     ///< Unique identifier inside current server
    uint8_t     connected:1;            ///< Connection establisched
    uint8_t     accepted:1;             ///< Incoming connection (i.e connection accepted)
    uint8_t     active:1;               ///< Node is active (i.e. runned)
    char        addr[1];                ///< Node address in following format: protocol://host:port
} mdv_node;


/// Link between nodes
typedef struct
{
    uint32_t    id[2];                  ///< Unique identifiers inside current server
    uint32_t    weight;                 ///< Link weight. Bigger is better.
} mdv_tracker_link;


/// Nodes and network topology tracker
typedef struct mdv_tracker mdv_tracker;


/**
 * @brief Create new topology tracker
 *
 * @param uid [in]              Global unique identifier for current node
 *
 * @return On success, return non-null pointer to tracker
 * @return On error, return NULL
 */
mdv_tracker * mdv_tracker_create(mdv_uuid const *uuid);


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
 * @brief Register peer connection
 *
 * @param tracker [in]          Topology tracker
 * @param new_node [in] [out]   node information
 *
 * @return On success, return MDV_OK and node->id is initialized by local unique numeric identifier
 * @return On error, return nonzero error code
 */
mdv_errno mdv_tracker_peer_connected(mdv_tracker *tracker, mdv_node *new_node);


/**
 * @brief Mark peer node as disconnected
 * @details Node is not deleted but only is marked as disconnected.
 *
 * @param tracker [in]  Topology tracker
 * @param uuid [in]     node UUID
 */
void mdv_tracker_peer_disconnected(mdv_tracker *tracker, mdv_uuid const *uuid);


/**
 * @brief node into the tracker
 *
 * @param tracker [in]      Topology tracker
 * @param node [in] [out]   node information
 * @param is_new [in]       New node flag (if it's true new identifier is generated)
 *
 * @return true if node was added
 */
bool mdv_tracker_append(mdv_tracker *tracker, mdv_node *node, bool is_new);


/**
 * @brief Iterate over all connected peers and call function fn.
 *
 * @param tracker [in]          Topology tracker
 * @param arg [in]              User defined data which is provided as second argument to the function fn
 * @param fn [in]               function pointer
 */
void mdv_tracker_peers_foreach(mdv_tracker *tracker, void *arg, void (*fn)(mdv_node *, void *));


/**
 * @brief Return active connected peers count.
 *
 * @param tracker [in]          Topology tracker
 *
 * @return peers count
 */
size_t mdv_tracker_peers_count(mdv_tracker *tracker);


/**
 * @brief Call function for node processing (e.g. post some message).
 *
 * @param tracker [in]          Topology tracker
 * @param id [in]               Unique peer identifier inside current server
 * @param arg [in]              User defined data which is provided as second argument to the function fn
 * @param fn [in]               function
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero error code
 */
mdv_errno mdv_tracker_peers_call(mdv_tracker *tracker, uint32_t id, void *arg, mdv_errno (*fn)(mdv_node *, void *));


/**
 * @brief Link state tracking between two peers.
 *
 * @param tracker [in]          Topology tracker
 * @param peer_1 [in]           First peer UUID
 * @param peer_2 [in]           Second peer UUID
 * @param connected [in]        Connection status
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero error code
 */
mdv_errno mdv_tracker_linkstate(mdv_tracker     *tracker,
                                mdv_uuid const  *peer_1,
                                mdv_uuid const  *peer_2,
                                bool             connected);


/**
 * @brief Link state tracking between two peers.
 *
 * @param tracker [in]          Topology tracker
 * @param link [in]             Link between two nodes
 * @param connected [in]        Connection status
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero error code
 */
mdv_errno mdv_tracker_linkstate2(mdv_tracker            *tracker,
                                 mdv_tracker_link const *link,
                                 bool                    connected);


/**
 * @brief Return links count in network topology.
 *
 * @param tracker [in]          Topology tracker
 *
 * @return links count
 */
size_t mdv_tracker_links_count(mdv_tracker *tracker);


/**
 * @brief Iterate over all topology links and call function fn.
 *
 * @param tracker [in]          Topology tracker
 * @param arg [in]              User defined data which is provided as second argument to the function fn
 * @param fn [in]               function pointer
 */
void mdv_tracker_links_foreach(mdv_tracker *tracker, void *arg, void (*fn)(mdv_tracker_link const *, void *));


/**
 * @brief Returns node uuid by local numeric id
 *
 * @param tracker [in]  Topology tracker
 * @param id [in]       Node identifier
 *
 * @return On success, returns non NULL node pointer.
 */
mdv_node * mdv_tracker_node_by_id(mdv_tracker *tracker, uint32_t id);


/**
 * @brief Returns node uuid by local numeric id
 *
 * @param tracker [in]  Topology tracker
 * @param uuid [in]     Node UUID
 *
 * @return On success, returns non NULL node pointer.
 */
mdv_node * mdv_tracker_node_by_uuid(mdv_tracker *tracker, mdv_uuid const *uuid);


/**
 * @brief Return node identifiers
 *
 * @param tracker [in]  Topology tracker
 *
 * @return node identifiers (vector<uint32_t>)
 */
mdv_vector * mdv_tracker_nodes(mdv_tracker *tracker);


/**
 * @brief Extract network topology from tracker
 * @details In result topology links are sorted in ascending order.
 *
 * @param tracker [in]          Topology tracker
 *
 * @return On success return non NULL pointer to a network topology.
 */
mdv_topology * mdv_tracker_topology(mdv_tracker *tracker);
