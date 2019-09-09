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
#include <mdv_msg.h>
#include "mdv_core.h"


/// Gossip peer identifier type
typedef uint32_t mdv_gossip_id;


/// Peer
typedef struct mdv_gossip_peer
{
    mdv_gossip_id    uid;       ///< Peers unique identifier
    uint32_t         lid;       ///< Peers local identifier
} mdv_gossip_peer;


/// Peers list
typedef struct mdv_gossip_peers
{
    uint32_t        size;       ///< Peers list size
    mdv_gossip_peer peers[1];   ///< Peers in ascending order of identifiers
} mdv_gossip_peers;


/**
 * @brief Broadcast the link state message to all cluster.
 *
 * @param core [in]             Core
 * @param src_peer [in]         First peer unique identifier
 * @param src_listen [in]       First peer listening address
 * @param dst_peer [in]         Second peer unique identifier
 * @param dst_listen [in]       Second peer listening address
 * @param connected [in]        1, if first peer connected to second peer
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero error code
 */
mdv_errno mdv_gossip_linkstate(mdv_core            *core,
                               mdv_uuid const      *src_peer,
                               char const          *src_listen,
                               mdv_uuid const      *dst_peer,
                               char const          *dst_listen,
                               bool                 connected);


/**
 * @brief Handle link state message.
 *
 * @param core [in]             Core
 * @param msg [in]              Incoming link state message
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero error code
 */
mdv_errno mdv_gossip_linkstate_handler(mdv_core *core, mdv_msg const *msg);
