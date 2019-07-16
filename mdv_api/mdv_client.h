/**
 * @file
 * @brief DB Client
 * @details Client provides API for DB accessing.
 */
#pragma once
#include <mdv_types.h>
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
mdv_client *mdv_client_connect(mdv_client_config const *config);


/**
 * @brief Close connection and free client
 *
 * @param client [in] DB client
 */
void mdv_client_close(mdv_client *client);


/**
 * @brief Create new table
 *
 * @param client [in]
 * @param table [in]
 *
 * @return On success, return MDV_OK. The table->uuid contains new table UUID.
 * @return On error, return non zero value
 */
mdv_errno mdv_create_table(mdv_client *client, mdv_table_base *table);

