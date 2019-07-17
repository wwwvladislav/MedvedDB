/**
 * @file
 * @brief Users connections processing logic.
 */
#pragma once
#include <mdv_uuid.h>
#include <mdv_errno.h>
#include <mdv_def.h>
#include <mdv_msg.h>
#include <mdv_string.h>
#include <mdv_limits.h>
#include <mdv_dispatcher.h>
#include "storage/mdv_tablespace.h"
#include "storage/mdv_nodes.h"


/// User context used for storing different type of information about connection (it should be cast to mdv_conctx)
typedef struct mdv_user mdv_user;


/**
 * @brief Initialize user
 *
 * @param tablespace [in] tables space
 * @param fd [in]         user socket descriptor
 * @param uuid [in]       current process uuid
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_user * mdv_user_accept(mdv_tablespace *tablespace, mdv_descriptor fd, mdv_uuid const *uuid);


/**
 * @brief   Read incoming messages
 *
 * @param user [in] user context
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_errno mdv_user_recv(mdv_user *user);


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
 * @param user [in]     user context
 */
void mdv_user_free(mdv_user *user);
