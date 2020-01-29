/**
 * @file
 * @brief Channels manager
 * @details Channels manager (chaman) is used for incoming and outgoing peers connections handling.
 */
#pragma once
#include "mdv_channel.h"
#include "mdv_string.h"
#include "mdv_threadpool.h"
#include "mdv_def.h"


/// Channels manager descriptor
typedef struct mdv_chaman mdv_chaman;


/// Connection handshake
typedef mdv_errno (*mdv_channel_handshake_fn)(mdv_descriptor fd,
                                              void          *userdata);


/// Connection accept handler
typedef mdv_errno  (*mdv_channel_accept_fn)(mdv_descriptor  fd,
                                            void           *userdata,
                                            mdv_channel_t  *channel_type,
                                            mdv_uuid       *uuid);


/// Channel creation
typedef mdv_channel * (*mdv_channel_create_fn)(mdv_descriptor    fd,
                                               void             *userdata,
                                               mdv_channel_t     channel_type,
                                               mdv_channel_dir   dir,
                                               mdv_uuid const   *channel_id);


/// Connection closing handler
typedef void (*mdv_channel_close_fn)(void *userdata, void *channel);


/// Channels manager configuration. All options are mandatory.
typedef struct
{
    struct
    {
        uint32_t                    retry_interval; ///< Interval between reconnections (in seconds)
        uint32_t                    keepidle;       ///< Start keeplives after this period (in seconds)
        uint32_t                    keepcnt;        ///< Number of keepalives before death
        uint32_t                    keepintvl;      ///< Interval between keepalives (in seconds)
        mdv_channel_handshake_fn    handshake;      ///< Connection handshake function
        mdv_channel_accept_fn       accept;         ///< Connection accept handler
        mdv_channel_create_fn       create;         ///< Channel creation function
    } channel;                                      ///< channel configuration
    mdv_threadpool_config   threadpool;             ///< thread pool options
    void                   *userdata;               ///< userdata passed to channel hadlers
} mdv_chaman_config;


/**
 * @brief Create new channels manager
 *
 * @param config [in] channels manager configuration
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
 * @brief Register new peer dialer
 *
 * @details Channels manager periodically tries to connect to this peer.
 *
 * @param chaman [in]   channels manager
 * @param addr [in]     peer address
 * @param type [in]     channel type
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_errno mdv_chaman_dial(mdv_chaman *chaman, mdv_string const addr, uint8_t type);


/**
 * @brief Unegister peer dialer (Not used. Without implementation.)
 *
 * @details Channels manager periodically tries to connect to this peer.
 *
 * @param chaman [in]   channels manager
 * @param addr [in]     peer address
 * @param type [in]     channel type
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_errno mdv_chaman_dial_stop(mdv_chaman *chaman, mdv_string const addr, uint8_t type);
