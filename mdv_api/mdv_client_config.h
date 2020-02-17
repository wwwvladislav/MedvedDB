/**
 * @file
 * @brief DB Client configuration
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <mdv_limits.h>


/// DB settings
typedef struct mdv_client_db_config
{
    char *addr;                     ///< DB address
} mdv_client_db_config;


/// Client connection configuration
typedef struct mdv_client_connection_config
{
    uint32_t    retry_interval;     ///< Interval between reconnections (in seconds)
    uint32_t    timeout;            ///< Connection timeout (in seconds)
    uint32_t    keepidle;           ///< Start keeplives after this period (in seconds)
    uint32_t    keepcnt;            ///< Number of keepalives before death
    uint32_t    keepintvl;          ///< Interval between keepalives (in seconds)
    uint32_t    response_timeout;   ///< Timeout for responses (in seconds)
} mdv_client_connection_config;


/// Client thread pool options
typedef struct mdv_client_threadpool_config
{
    size_t      size;               ///< threads count in thread pool
} mdv_client_threadpool_config;


/// Client configuration. All options are mandatory.
typedef struct
{
    mdv_client_db_config            db;             ///< DB settings
    mdv_client_connection_config    connection;     ///< connection configuration
    mdv_client_threadpool_config    threadpool;     ///< thread pool options
} mdv_client_config;
