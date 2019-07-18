/**
 * @file
 * @brief Cluster manager
 */

#pragma once
#include "storage/mdv_tablespace.h"
#include "mdv_tracker.h"
#include <mdv_chaman.h>


/// Cluster manager
typedef struct mdv_cluster
{
    mdv_uuid        uuid;       ///< Current server UUID
    mdv_tablespace *tablespace; ///< Tables storage
    mdv_tracker     tracker;    ///< Topology tracker
    mdv_chaman     *chaman;     ///< Channels manager
} mdv_cluster;


/**
 * @brief Create cluster manager
 *
 * @param cluster [out]     cluster manager to be initialized
 * @param tablespace [in]   tables storage
 * @param nodes [in]        nodes storage
 * @param uuid [in]         current server UUID
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero value
 */
mdv_errno mdv_cluster_create(mdv_cluster *cluster, mdv_tablespace *tablespace, mdv_nodes *nodes, mdv_uuid const *uuid);


/**
 * @brief Free cluster manager
 *
 * @param cluster [in] cluster manager
 */
void mdv_cluster_free(mdv_cluster *cluster);


/**
 * @brief Update cluster nodes
 *
 * @param cluster [in] cluster manager
 */
void mdv_cluster_update(mdv_cluster *cluster);
