/**
 * @file
 * @brief Cluster manager
 */

#pragma once
#include "mdv_tracker.h"
#include "mdv_chaman.h"
#include "mdv_dispatcher.h"
#include "mdv_hashmap.h"


/// Cluster manager
typedef struct mdv_cluster mdv_cluster;


/// Connection context
typedef struct
{
    uint8_t             type;                       ///< Connection context type
    uint8_t             reserved[7];                ///< Reserved
    mdv_dispatcher     *dispatcher;                 ///< Messages dispatcher
    size_t              created_time;               ///< Time, when peer registered
    mdv_cluster        *cluster;                    ///< Cluster manager

    /**
     * @brief Peer registration
     *
     * @param cluster [in]  Cluster manager
     * @param addr [in]     peer node listen address
     * @param uuid [in]     peer global UUID
     * @param id [out]      peer local numeric identifier
     *
     * @return On success, return MDV_OK
     * @return On error, return non zero value
     */
    mdv_errno (*reg_peer)(mdv_cluster *cluster, char const *addr, mdv_uuid const *uuid, uint32_t *id);

    /**
     * @brief Peer unregistration
     *
     * @param cluster [in]  Cluster manager
     * @param uuid [in]     peer global UUID
     */
    void (*unreg_peer)(mdv_cluster *cluster, mdv_uuid const *uuid);

    uint8_t dataspace[1];                           ///< Data space is used to place data associated with connection
} mdv_conctx;


/// Connection context configuration
typedef struct
{
    uint8_t     type;                                                   ///< Connection context type
    size_t      size;                                                   ///< Connection context size
    mdv_errno (*init)(void *ctx, mdv_conctx *conctx, void *userdata);   ///< Connection context initialization function
    void (*free)(void *ctx, mdv_conctx *conctx);                        ///< Connection context freeing function
} mdv_conctx_config;


/// Cluster manager configuration. All options are mandatory.
typedef struct
{
    mdv_uuid        uuid;                   ///< Current cluster node UUID

    struct
    {
        uint32_t    keepidle;               ///< Start keeplives after this period (in seconds)
        uint32_t    keepcnt;                ///< Number of keepalives before death
        uint32_t    keepintvl;              ///< Interval between keepalives (in seconds)
    } channel;                              ///< Channel configuration

    mdv_threadpool_config   threadpool;     ///< Thread pool options

    struct
    {
        void                    *userdata;  ///< Userdata which is provided as argument to connection context initialization function
        size_t                   size;      ///< Connection contexts types count
        mdv_conctx_config const *configs;   ///< Connection contexts configurations
    } conctx;                               ///< Connection contexts settings
} mdv_cluster_config;


/// Cluster manager
typedef struct mdv_cluster
{
    mdv_uuid        uuid;       ///< Current cluster node UUID
    mdv_tracker     tracker;    ///< Topology tracker
    mdv_hashmap     conctx_cfgs;///< Connection contexts configurations
    mdv_chaman     *chaman;     ///< Channels manager
    void           *userdata;   ///< Userdata which is provided as argument to connection context initialization function
} mdv_cluster;


/**
 * @brief Create cluster manager
 *
 * @param cluster [out]     cluster manager to be initialized
 * @param config [in]       cluster manager configuration
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero error code
 */
mdv_errno mdv_cluster_create(mdv_cluster *cluster, mdv_cluster_config const *config);


/**
 * @brief Free cluster manager
 *
 * @param cluster [in] cluster manager
 */
void mdv_cluster_free(mdv_cluster *cluster);


/**
 * @brief Bind cluster to the specified address
 *
 * @param cluster [in]  cluster manager
 * @param addr [in]     address for bind
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero error code
 */
mdv_errno mdv_cluster_bind(mdv_cluster *cluster, mdv_string const addr);


/**
 * @brief Connect to the specified address
 *
 * @param cluster [in]  cluster manager
 * @param addr [in]     address for bind
 * @param type [in]     connection type
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero error code
 */
mdv_errno mdv_cluster_connect(mdv_cluster *cluster, mdv_string const addr, uint8_t type);
