#pragma once
#include <mdv_uuid.h>
#include <mdv_errno.h>
#include <mdv_def.h>
#include <stdatomic.h>


/// Peer context used for storing different type of information about active peer
typedef struct mdv_peer
{
    mdv_uuid        uuid;               ///< peer uuid
    mdv_descriptor  fd;                 ///< file decriptoir associated with peer
    atomic_ushort   req_id;             ///< request id
    size_t          created_time;       ///< time, when peer regictered
    uint32_t        initialized:1;      ///< initialization flag (while hello message is not processed, peer is not valid)
} mdv_peer;


/**
 * @brief   Say Hey! to peer node
 *
 * @param peer [in] peer node
 * @param uuid [in] current process uuid
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_errno mdv_peer_wave(mdv_peer *peer, mdv_uuid const *uuid);

