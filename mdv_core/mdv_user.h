/**
 * @file
 * @brief Users connections processing logic.
 */
#pragma once
#include <mdv_uuid.h>
#include <mdv_def.h>
#include <mdv_msg.h>
#include <mdv_string.h>
#include <mdv_limits.h>
#include <mdv_cluster.h>
#include "mdv_service.h"


/// User context used for storing different type of information about connection (it should be cast to mdv_conctx)
typedef struct mdv_user
{
    mdv_service     *service;               ///< service
    mdv_conctx      *conctx;                ///< connection context
} mdv_user;

/**
 * @brief Initialize user
 *
 * @param ctx [in]      user context
 * @param conctx [in]   connection context
 * @param userdata [in] pointer to mdv_tablespace
 *
 * @return On success, return pointer to new user connection context
 * @return On error, return NULL pointer
 */
mdv_errno mdv_user_init(void *ctx, mdv_conctx *conctx, void *userdata);


/**
 * @brief Send message and wait response.
 *
 * @param user [in]     user context
 * @param req [in]      request to be sent
 * @param resp [out]    received response
 * @param timeout [in]  timeout for response wait (in milliseconds)
 *
 * @return MDV_OK if message is successfully sent and 'resp' contains response from remote user
 * @return MDV_BUSY if there is no free slots for request. At this case caller should wait and try again later.
 * @return On error return nonzero error code
 */
mdv_errno mdv_user_send(mdv_user *user, mdv_msg *req, mdv_msg *resp, size_t timeout);


/**
 * @brief Send message but response isn't required.
 *
 * @param user [in]     user context
 * @param msg [in]      message to be sent
 *
 * @return On success returns MDV_OK
 * @return On error return nonzero error code.
 */
mdv_errno mdv_user_post(mdv_user *user, mdv_msg *msg);


/**
 * @brief Free message
 *
 * @param ctx [in]     user context
 * @param conctx [in]  connection context
 */
void mdv_user_free(void *ctx, mdv_conctx *conctx);
