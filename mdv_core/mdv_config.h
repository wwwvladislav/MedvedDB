/**
 * @file
 * @brief Server configuration
 * @details This file contains server configuration parameters.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <mdv_string.h>
#include <mdv_stack.h>


enum
{
    MDV_MAX_CLUSTER_SIZE = 256      ///< Limit for cluster size
};


/// Server configuration
typedef struct
{
    struct
    {
        mdv_string level;       ///< Log output level
    } log;                      ///< Log settings

    struct
    {
        mdv_string listen;          ///< Server IP and port for listening
        uint32_t   workers;         ///< Number of thread pool workers for incoming requests processing
    } server;                       ///< Server settings

    struct
    {
        uint32_t retry_interval;    ///< Interval between reconnections (in seconds)
        uint32_t keep_idle;         ///< Start keeplives after this period (in seconds)
        uint32_t keep_count;        ///< Number of keepalives before death
        uint32_t keep_interval;     ///< Interval between keepalives (in seconds)
    } connection;                   ///< Connection settings

    struct
    {
        mdv_string path;            ///< Directory where the database is placed
        mdv_string trlog;           ///< Directory where the database transaction logs are placed
        mdv_string rowdata;         ///< Directory where the database rowdata is placed
    } storage;                      ///< Storage settings

    struct
    {
        uint32_t   workers;         ///< Number of thread pool workers for events processing
        uint32_t   queues;          ///< Number of event queues
    } ebus;

    struct
    {
        uint32_t   workers;         ///< Number of thread pool workers for transaction log applying
        uint32_t   queues;          ///< Number of event queues
        uint32_t   batch_size;      ///< Batch size for data commit
    } committer;

    struct
    {
        uint32_t   workers;         ///< Number of thread pool workers for data synchronization
        uint32_t   queues;          ///< Number of event queues
        uint32_t   batch_size;      ///< Batch size for data synchronization
    } datasync;                     ///< Data synchronizer settings

    struct
    {
        uint32_t   workers;         ///< Number of thread pool workers for data fetching from database
        uint32_t   queues;          ///< Number of event queues
        uint32_t   batch_size;      ///< Batch size for data fetching
        uint32_t   vm_stack;        ///< VM stack size
        uint32_t   views_lifetime;  ///< Inactive views lifetime (in seconds)
    } fetcher;                      ///< Data fetcher settings

    struct
    {
        uint32_t    size;           ///< Nimber of defined cluster nodes
        char const *nodes[MDV_MAX_CLUSTER_SIZE];    ///< Defined cluster nodes
    } cluster;                      ///< Cluster settings

    mdv_stack(char, 4 * 1024) mempool;   ///< Memory pool for strings allocation
} mdv_config;


/// Global server configuration
extern mdv_config MDV_CONFIG;

/**
 * @brief Load configuration from file
 *
 * @param path [in] Configuration file path
 *
 * @return On success returns true
 * @return On error returns false
 */
bool mdv_load_config(char const *path);

