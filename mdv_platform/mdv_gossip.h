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

#include "mdv_msg.h"


/// Gossip peer identifier type
typedef uint32_t mdv_gossip_id;


/// Peers list
typedef struct mdv_gossip_peers
{
    uint32_t         size;          ///< Peers list size
    mdv_gossip_id   *ids;           ///< Peers unique identifiers in ascending order
    void           **data;          ///< Data associated with peer
} mdv_gossip_peers;


/**
 * @brief Broadcast message using gossip protocol
 * 
 * @param msg [in]
 * @param src [in]
 * @param dst [in]
 * @param post [in]
 *
 * @return MDV_OK if message is successfully broadcasted
 * @return non zero error code if error has occurred
 */
mdv_errno mdv_gossip_broadcast(mdv_msg const *msg,
                               mdv_gossip_peers const *src,
                               mdv_gossip_peers const *dst,
                               mdv_errno (*post)(void *data, mdv_msg *msg));
