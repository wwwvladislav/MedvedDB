/**
 * @file mdv_gossip.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Gossip protocol implementation for message broadcasting.
 * @version 0.1
 * @date 2019-07-29
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_tracker.h>


/// Gossip peer identifier type
typedef uint32_t mdv_gossip_id;


/// Peer
typedef struct mdv_gossip_peer
{
    uint32_t         uid;       ///< Peers unique identifier
    uint32_t         lid;       ///< Peers local identifier
} mdv_gossip_peer;


/// Peers list
typedef struct mdv_gossip_peers
{
    uint32_t        size;       ///< Peers list size
    mdv_gossip_peer peers[1];   ///< Peers in ascending order of identifiers
} mdv_gossip_peers;


/**
 * @brief s
 *
 * @param tracker [in]          Topology tracker
 * @param peer_1 [in]           First peer unique identifier
 * @param peer_2 [in]           Second peer unique identifier
 * @param connected [in]        1, if first peer connected to second peer
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero error code
 */
mdv_errno mdv_gossip_linkstate(mdv_tracker *tracker, mdv_uuid const *peer_1, mdv_uuid const *peer_2, bool connected);
