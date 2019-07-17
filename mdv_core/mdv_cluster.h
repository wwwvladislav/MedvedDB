/**
 * @file
 * @brief Cluster manager
 */

#pragma once
#include "storage/mdv_nodes.h"
#include "storage/mdv_tablespace.h"


/// Cluster manager
typedef struct mdv_cluster mdv_cluster;


/**
 * @brief Create cluster manager
 *
 * @param tablespace [in]   tables storage
 * @param nodes [in]        nodes storage
 * @param uuid [in]         current server UUID
 *
 * @return On success, return pointer to new cluster manager
 * @return On error, return NULL pointer
 */
mdv_cluster * mdv_cluster_create(mdv_tablespace *tablespace, mdv_nodes *nodes, mdv_uuid const *uuid);


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
