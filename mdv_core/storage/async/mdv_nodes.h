/**
 * @file mdv_nodes.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Asynchronous version of cluster nodes storage API.
 * @version 0.1
 * @date 2019-08-02
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_jobber.h>
#include "../mdv_nodes.h"


/**
 * @brief Store new node in storage
 *
 * @param jobber [in]   jobs scheduler
 * @param storage [in]  storage where nodes saved
 * @param node [in]     node information
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero value
 */
mdv_errno mdv_nodes_store_async(mdv_jobber *jobber, mdv_storage *storage, mdv_node const *node);
