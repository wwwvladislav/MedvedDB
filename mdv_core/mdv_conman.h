/**
 * @file
 * @brief Connections manager
 */

#pragma once
#include <mdv_def.h>
#include <mdv_uuid.h>
#include <mdv_threadpool.h>
#include <mdv_ebus.h>


/// Connections manager
typedef struct mdv_conman mdv_conman;


/// Connection manager configuration. All options are mandatory.
typedef struct
{
    mdv_uuid        uuid;                   ///< Current node UUID

    struct
    {
        uint32_t    keepidle;               ///< Start keeplives after this period (in seconds)
        uint32_t    keepcnt;                ///< Number of keepalives before death
        uint32_t    keepintvl;              ///< Interval between keepalives (in seconds)
    } channel;                              ///< Channel configuration

    mdv_threadpool_config   threadpool;     ///< Thread pool options
} mdv_conman_config;


/**
 * @brief Create connections manager
 *
 * @param conman_config [in]    connections manager configuration
 * @param ebus [in]             events bus
 *
 * @return On success, returns nonsero pointer to new connections manager
 * @return On error, return nonzero error code
 */
mdv_conman * mdv_conman_create(mdv_conman_config const *conman_config, mdv_ebus *ebus);


/**
 * @brief Free connections manager
 *
 * @param conman [in] connections manager
 */
void mdv_conman_free(mdv_conman *conman);


/**
 * @brief Bind server to the specified address
 *
 * @param conman [in]   connections manager
 * @param addr [in]     address for bind
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero error code
 */
mdv_errno mdv_conman_bind(mdv_conman *conman, mdv_string const addr);


/**
 * @brief Connect to the specified address
 *
 * @param conman [in]   connections manager
 * @param addr [in]     address for bind
 * @param type [in]     connection type
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero error code
 */
mdv_errno mdv_conman_connect(mdv_conman *conman, mdv_string const addr, uint8_t type);
