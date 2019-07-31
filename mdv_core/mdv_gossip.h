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
    uint32_t         size;          ///< Peers list size
    mdv_gossip_peer *peers;         ///< Peers in ascending order of identifiers
} mdv_gossip_peers;


void mdv_gossip_peers_get(mdv_tracker *tracker, mdv_gossip_peers *peers);
