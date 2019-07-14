/**
 * @file
 * @brief Remote peers connections processing logic.
 * @details Remote peer might be a user or other cluster node.
 */
#pragma once
#include <mdv_uuid.h>
#include <mdv_errno.h>
#include <mdv_def.h>
#include <stdatomic.h>
#include <mdv_msg.h>
#include <mdv_string.h>
#include <mdv_limits.h>
#include "storage/mdv_tablespace.h"
#include "storage/mdv_nodes.h"


/// Peer context used for storing different type of information about active peer
typedef struct mdv_peer
{
    mdv_tablespace     *tablespace;             ///< tablespace
    mdv_nodes          *nodes;                  ///< nodes storage
    mdv_uuid            uuid;                   ///< peer uuid
    uint32_t            id;                     ///< Unique peer identifier inside current server
    mdv_descriptor      fd;                     ///< file decriptoir associated with peer
    mdv_string          addr;                   ///< peer address
    atomic_ushort       id_gen;                 ///< id generator
    size_t              created_time;           ///< time, when peer regictered
    volatile uint32_t   initialized:1;          ///< initialization flag (while hello message is not processed, peer is not valid)
    mdv_msg             message;                ///< incomming message
    char                buff[MDV_ADDR_LEN_MAX]; ///< buffer for address string
} mdv_peer;


/**
 * @brief   Initialize peer
 *
 * @param peer [in] peer node
 * @param tablespace [in] tables space
 * @param nodes [in] nodes storage
 * @param fd [in] peer socket descriptor
 * @param addr [in] peer address
 * @param uuid [in] current process uuid
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_errno mdv_peer_init(mdv_peer *peer, mdv_tablespace *tablespace, mdv_nodes *nodes, mdv_descriptor fd, mdv_string const *addr, mdv_uuid const *uuid);


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
