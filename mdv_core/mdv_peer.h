/**
 * @file
 * @brief Remote peers connections processing logic.
 * @details Remote peer might be a user or other cluster node.
 */
#pragma once
#include <mdv_uuid.h>
#include <mdv_def.h>
#include <mdv_chaman.h>
#include <mdv_msg.h>


/// Peer context used for storing different type of information
/// about active peer (it should be cast to mdv_conctx)
typedef struct mdv_peer mdv_peer;


/**
 * @brief Creates peer connection context
 *
 * @param uuid [in]     current node uuid
 * @param dir [in]      channel direction
 * @param fd [in]       channel descriptor
 *
 * @return On success, return pointer to new peer context
 * @return On error, return NULL pointer
 */
mdv_peer * mdv_peer_create(mdv_uuid const *uuid,
                           mdv_channel_dir dir,
                           mdv_descriptor fd);


/**
 * @brief Retains peer connection context.
 * @details Reference counter is increased by one.
 */
mdv_peer * mdv_peer_retain(mdv_peer *peer);


/**
 * @brief Retains peer connection context.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the  peer's connection context destructor is called.
 */
uint32_t mdv_peer_release(mdv_peer *peer);


/**
 * @brief Send message and wait response.
 *
 * @param peer [in]     peer connection context
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
 * @param peer [in]     peer connection context
 * @param msg [in]      message to be sent
 *
 * @return On success returns MDV_OK
 * @return On error return nonzero error code.
 */
mdv_errno mdv_peer_post(mdv_peer *peer, mdv_msg *msg);


/**
 * @brief Reads incomming messages
 *
 * @param peer [in]     peer connection context
 *
 * @return On success returns MDV_OK
 * @return On error return nonzero error code.
 */
mdv_errno mdv_peer_recv(mdv_peer *peer);


/**
 * @brief Returns peer channel descriptor
 *
 * @param peer [in]     peer connection context
 */
mdv_descriptor mdv_peer_fd(mdv_peer *peer);
