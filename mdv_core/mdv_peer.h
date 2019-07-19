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
#include <mdv_dispatcher.h>
#include "storage/mdv_tablespace.h"
#include "mdv_conctx.h"


/// Peer context used for storing different type of information about active peer (it should be cast to mdv_conctx)
typedef struct mdv_peer mdv_peer;


/**
 * @brief Initialize incoming peer
 *
 * @param config [in] Connection context configuration
 *
 * @return On success, return pointer to new peer context
 * @return On error, return NULL pointer
 */
mdv_peer * mdv_peer_accept(mdv_conctx_config const *config);


/**
 * @brief Initialize outgoing peer
 *
 * @param config [in] Connection context configuration
 *
 * @return On success, return pointer to new peer context
 * @return On error, return NULL pointer
 */
mdv_peer * mdv_peer_connect(mdv_conctx_config const *config);


/**
 * @brief   Read incoming messages
 *
 * @param peer [in] peer node
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_errno mdv_peer_recv(mdv_peer *peer);


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
 * @brief Free message
 *
 * @param peer [in] peer node
 */
void mdv_peer_free(mdv_peer *peer);
