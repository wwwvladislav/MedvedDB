/**
 * @file
 * @brief Channels manager
 * @details Channels manager (chaman) is used for incoming and outgoing peers connections handling.
 */
#pragma once
#include "mdv_errno.h"
#include "mdv_string.h"
#include "mdv_threadpool.h"
#include "mdv_def.h"


/// Channels manager descriptor
typedef struct mdv_chaman mdv_chaman;


/// Connection type select handler
typedef mdv_errno (*mdv_channel_select_fn)(mdv_descriptor fd, uint32_t *type);


/// Channel direction
typedef enum
{
    MDV_CHIN = 0,       ///< Incomming connection
    MDV_CHOUT           ///< Outgoing connection
} mdv_channel_dir;


/// Connection creation handler
typedef void * (*mdv_channel_create_fn)(mdv_descriptor fd,
                                        mdv_string const *addr,
                                        void *userdata,
                                        uint32_t type,
                                        mdv_channel_dir dir);


/// Data receiving handler
typedef mdv_errno (*mdv_channel_recv_fn)(void *channel);


/// Connection closing handler
typedef void (*mdv_channel_close_fn)(void *channel);


/// Channels manager configuration. All options are mandatory.
typedef struct
{
    struct
    {
        uint32_t    keepidle;           ///< Start keeplives after this period (in seconds)
        uint32_t    keepcnt;            ///< Number of keepalives before death
        uint32_t    keepintvl;          ///< Interval between keepalives (in seconds)
    } peer;                             ///< peer configuration

    mdv_threadpool_config   threadpool; ///< thread pool options

    void                   *userdata;   ///< userdata passed to channel hadlers

    struct
    {
        mdv_channel_select_fn   select; ///< channel selecting function (return channel type)
        mdv_channel_create_fn   create; ///< channel creation function
        mdv_channel_recv_fn     recv;   ///< data receiving handler
        mdv_channel_close_fn    close;  ///< channel closing function
    } channel;                          ///< channel configuration
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
 * @brief Register new peer
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
mdv_errno mdv_chaman_connect(mdv_chaman *chaman, mdv_string const addr, uint32_t type);

