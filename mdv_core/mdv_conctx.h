/**
 * @file
 * @brief Base type for connection contexts
 */

#pragma once
#include "storage/mdv_tablespace.h"
#include <mdv_messages.h>
#include <mdv_def.h>


/// Base type for connection contexts mdv_peer and mdv_user
typedef struct mdv_conctx
{
    mdv_cli_type type;                          ///< Client type
} mdv_conctx;


/// Connection context event handlers
typedef struct mdv_conctx_handlers
{
    /**
     * @brief Peer registration
     *
     * @param userdata [in] user data
     * @param addr [in]     peer node listen address
     * @param uuid [in]     peer global UUID
     * @param id [out]      peer local numeric identifier
     *
     * @return On success, return MDV_OK
     * @return On error, return non zero value
     */
    mdv_errno (*reg_peer)(void *userdata, char const *addr, mdv_uuid const *uuid, uint32_t *id);


    /**
     * @brief Peer unregistration
     *
     * @param userdata [in] user data
     * @param uuid [in]     peer global UUID
     */
    void (*unreg_peer)(void *userdata, mdv_uuid const *uuid);
} mdv_conctx_handlers;


/// Connection context configuration
typedef struct mdv_conctx_config
{
    mdv_tablespace     *tablespace;     ///< DB tables space
    mdv_descriptor      fd;             ///< file descriptor associated with connection
    mdv_uuid            uuid;           ///< current server UUID
    void               *userdata;       ///< User defined data which is provided as event handlers first argument
    mdv_conctx_handlers handlers;       ///< Event handlers and callbacks
} mdv_conctx_config;


/**
 * @brief Select connection context
 *
 * @param fd [in]       file descriptor
 * @param type [out]    connection context type
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_errno mdv_conctx_select(mdv_descriptor fd, uint32_t *type);


/**
 * @brief Create connection context
 *
 * @param config [in]   Connection context configuration
 * @param type [in]     Client type (MDV_CLI_USER or MDV_CLI_PEER)
 * @param dir [in]      Channel direction (MDV_CHIN or MDV_CHOUT)
 *
 * @return On success, return new connection context
 * @return On error, return NULL pointer
 */
mdv_conctx * mdv_conctx_create(mdv_conctx_config const *config, uint32_t type, uint32_t dir);


/**
 * @brief Read incoming data
 *
 * @param conctx [in] connection context
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_errno mdv_conctx_recv(mdv_conctx *conctx);


/**
 * @brief Free allocated by connection context resources
 *
 * @param conctx [in] connection context
 */
void mdv_conctx_free(mdv_conctx *conctx);

