#pragma once
#include <mdv_uuid.h>
#include <mdv_errno.h>
#include <mdv_def.h>
#include <stdatomic.h>
#include <mdv_msg.h>


/// Peer context used for storing different type of information about active peer
typedef struct mdv_peer
{
    mdv_uuid            uuid;           ///< peer uuid
    mdv_descriptor      fd;             ///< file decriptoir associated with peer
    atomic_ushort       id;             ///< id generator
    size_t              created_time;   ///< time, when peer regictered
    volatile uint32_t   initialized:1;  ///< initialization flag (while hello message is not processed, peer is not valid)
    mdv_msg             message;        ///< incomming message
} mdv_peer;


/**
 * @brief   Initialize peer
 *
 * @param peer [in] peer node
 * @param fd [in] peer socket descriptor
 * @param uuid [in] current process uuid
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_errno mdv_peer_init(mdv_peer *peer, mdv_descriptor fd, mdv_uuid const *uuid);


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
 * @brief   Send message
 *
 * @param peer [in] peer node
 * @param message [in] message to be sent
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_errno mdv_peer_send(mdv_peer *peer, mdv_msg const *message);


/**
 * @brief Free message
 *
 * @param peer [in] peer node
  */
void mdv_peer_free(mdv_peer *peer);
