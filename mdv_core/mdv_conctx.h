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
    mdv_errno (*recv)(struct mdv_conctx *);     ///< function for data receiving
    void (*close)(struct mdv_conctx *);         ///< function for connection context closing
} mdv_conctx;


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
 * @param tablespace [in]   tables storage
 * @param fd [in]           file descriptor associated with connection
 * @param uuid [in]         current server UUID
 * @param type [in]         client type (MDV_CLI_USER or MDV_CLI_PEER)
 * @param dir [in]          Channel direction (MDV_CHIN or MDV_CHOUT)
 *
 * @return On success, return new connection context
 * @return On error, return NULL pointer
 */
void * mdv_conctx_create(mdv_tablespace *tablespace, mdv_descriptor fd, mdv_uuid const *uuid, uint32_t type, uint32_t dir);


/**
 * @brief Read incoming data
 *
 * @param conctx [in] connection context
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_errno mdv_conctx_recv(void *conctx);


/**
 * @brief Close connection context
 *
 * @param conctx [in] connection context
 */
void mdv_conctx_close(void *conctx);

