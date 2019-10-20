/**
 * @file mdv_node.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Node definition
 * @version 0.1
 * @date 2019-10-20
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_def.h>
#include <mdv_uuid.h>


enum
{
    MDV_LOCAL_ID = 0                    ///< Local identifier for current node
};


/// Cluster node information
typedef struct
{
    mdv_uuid    uuid;                   ///< Global unique identifier
    uint32_t    id;                     ///< Unique identifier inside current server
    char        addr[1];                ///< Node address in following format: protocol://host:port
} mdv_node;


/**
 * @brief Calculates memory size required for node
 */
size_t mdv_node_size(mdv_node const *node);
