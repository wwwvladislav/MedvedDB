/**
 * @file
 * @brief DB Client
 * @details Client provides API for DB accessing.
 */
#pragma once
#include <mdv_types.h>
#include <mdv_table.h>
#include <mdv_topology.h>
#include <mdv_errno.h>


/// Client configuration. All options are mandatory.
typedef struct
{
    struct
    {
        char const *addr;               ///< DB address
    } db;                               ///< DB settings

    struct
    {
        uint32_t    timeout;            ///< Connection timeout (in seconds)
        uint32_t    keepidle;           ///< Start keeplives after this period (in seconds)
        uint32_t    keepcnt;            ///< Number of keepalives before death
        uint32_t    keepintvl;          ///< Interval between keepalives (in seconds)
        uint32_t    response_timeout;   ///< Timeout for responses (in seconds)
    } connection;                       ///< connection configuration

    struct
    {
        size_t      size;               ///< threads count in thread pool
    } threadpool;                       ///< thread pool options
} mdv_client_config;


/// Client descriptor
typedef struct mdv_client mdv_client;


/**
 * @brief Connect to DB
 *
 * @param config [in] client configuration
 *
 * @return On success, return nonzero client pointer
 * @return On error, return NULL pointer
 */
mdv_client * mdv_client_connect(mdv_client_config const *config);


/**
 * @brief Close connection and free client
 *
 * @param client [in] DB client
 */
void mdv_client_close(mdv_client *client);


/**
 * @brief Create new table
 *
 * @param client [in]   DB client
 * @param table  [in]   Table description
 *               [out]  Table identifier (table->id)
 *
 * @return On success, returns nonsero table descriptor.
 * @return On error, returns NULL
 */
mdv_table * mdv_create_table(mdv_client *client, mdv_table_desc *table);


/**
 * @brief Request network topology.
 * @details This function should be used only for diagnostic purposes. Use mdv_free() to free the allocated links array.
 *
 * @param client [in]       DB client
 * @param topology [out]    Topology
 *
 * @return On success, return MDV_OK.
 * @return On error, return non zero value
 */
mdv_errno mdv_get_topology(mdv_client *client, mdv_topology **topology);


// mdv_errno mdv_get_table_uuids(mdv_client *client, char const *name, mdv_list * info);

// mdv_table * mdv_get_table(mdv_client *client, mdv_uuid const *uuid);


/**
 * @brief Insert row to given table
 * @details This function implements insertion functionality.
 *
 * @param client [in]    DB client
 * @param table_id [in]    The guid of table
 * @param row [in]    Row description
 * @param id [out]    Row identifier
 *
 * @return On success, return MDV_OK.
 * @return On error, return non zero value
 */
mdv_errno mdv_insert_row(mdv_client *client, mdv_gobjid const *table_id, mdv_field const *fields, mdv_row_base const *row, mdv_gobjid *id);
