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


