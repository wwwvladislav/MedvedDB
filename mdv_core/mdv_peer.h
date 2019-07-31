/**
 * @file
 * @brief Remote peers connections processing logic.
 * @details Remote peer might be a user or other cluster node.
 */
#pragma once
#include <mdv_uuid.h>
#include <mdv_def.h>
#include <mdv_msg.h>
#include <mdv_string.h>
#include <mdv_limits.h>
#include <mdv_cluster.h>
#include "mdv_core.h"


/// Peer context used for storing different type of information about active peer (it should be cast to mdv_conctx)
typedef struct mdv_peer
{
    mdv_core           *core;                       ///< core
    mdv_conctx         *conctx;                     ///< connection context
    uint32_t            peer_id;                    ///< peer local numeric id
    mdv_uuid            peer_uuid;                  ///< peer global uuid
} mdv_peer;


/**
 * @brief Initialize peer
 *
 * @param ctx [in]      peer context
 * @param conctx [in]   connection context
 * @param userdata [in] pointer to mdv_tablespace
 *
 * @return On success, return pointer to new peer context
 * @return On error, return NULL pointer
 */
mdv_errno mdv_peer_init(void *ctx, mdv_conctx *conctx, void *userdata);


/**
 * @brief Send message and wait response.
 *
 * @param peer [in]     peer node
 * @param req [in]      request to be sent
 * @param resp [out]    received response
 * @param timeout [in]  timeout for response wait (in milliseconds)
 *
 * @return MDV_OK if message is successfully sent and 'resp' contains response from remote peer
 * @return MDV_BUSY if there is no free slots for request. At this case caller should wait and try again later.
 * @return On error return nonzero error code
 */
mdv_errno mdv_peer_send(mdv_peer *peer, mdv_msg *req, mdv_msg *resp, size_t timeout);


/**
 * @brief Send message but response isn't required.
 *
 * @param peer [in]     peer node
 * @param msg [in]      message to be sent
 *
 * @return On success returns MDV_OK
 * @return On error return nonzero error code.
 */
mdv_errno mdv_peer_post(mdv_peer *peer, mdv_msg *msg);


/**
 * @brief Peer context freeing function
 *
 * @param ctx [in]     user context
 * @param conctx [in]  connection context
 */
void mdv_peer_free(void *ctx, mdv_conctx *conctx);
