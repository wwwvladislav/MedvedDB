/**
 * @file
 * @brief Channels manager
 * @details Channels manager (chaman) is used for incoming and outgoing peers connections handling.
 */
#pragma once
#include <stddef.h>
#include "mdv_errno.h"
#include "mdv_string.h"
#include "mdv_uuid.h"


/// Channels manager descriptor
typedef struct mdv_chaman mdv_chaman;


/// Unique peer identifier
typedef mdv_uuid mdv_peer_id;


/// Channels manager configuration
typedef struct
{
    size_t          tp_size;            ///< threads count in thread pool
    mdv_peer_id     uuid;               ///< current peer identifier

    struct
    {
        size_t      reconnect_timeout;  ///< reconnection timeout to the peer (in milliseconds)
        size_t      keepalive_timeout;  ///< timeout for keepalive packages send (in milliseconds)
    } peer;                             ///< peer configuration
} mdv_chaman_config;


/**
 * @brief Create new channels manager
 *
 * @param tp_size [in] threads count in thread pool
 *
 * @return On success, return pointer to a new created channels manager
 * @return On error, return NULL
 */
mdv_chaman * mdv_chaman_create(mdv_chaman_config const *config);


/**
 * @brief Stop and free channels manager
 *
 * @param chaman [in] channels manager
 */
void mdv_chaman_free(mdv_chaman *chaman);


/**
 * @brief Register new listener
 *
 * @param chaman [in] channels manager
 * @param addr [in] address for listening in following format protocol://host:port.
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_errno mdv_chaman_listen(mdv_chaman *chaman, mdv_string const addr);


/**
 * @brief Register new peer
 *
 * @details Channels manager periodically tries to connect to this peer.
 *
 * @param chaman [in] channels manager
 * @param addr [in] peer address
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_errno mdv_chaman_connect(mdv_chaman *chaman, mdv_string const addr);

