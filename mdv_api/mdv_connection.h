/**
 * @file
 * @brief Users connections processing logic.
 */
#pragma once
#include <mdv_def.h>
#include <mdv_msg.h>
#include <mdv_uuid.h>


/// User connection context used for storing different type of information
/// about connection
typedef struct mdv_connection mdv_connection;


/**
 * @brief Initialize user
 *
 * @param fd [in]           channel file descriptor
 *
 * @return On success, return pointer to new user connection context
 * @return On error, return NULL pointer
 */
mdv_connection * mdv_connection_create(mdv_descriptor fd);


/**
 * @brief Retains user connection context.
 * @details Reference counter is increased by one.
 */
mdv_connection * mdv_connection_retain(mdv_connection *channel);


/**
 * @brief Retains user connection context.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the  user's connection context destructor is called.
 */
uint32_t mdv_connection_release(mdv_connection *channel);


/**
 * @brief Returns channel UUID
 */
mdv_uuid const * mdv_connection_uuid(mdv_connection *channel);


/**
 * @brief Send message and wait response.
 *
 * @param channel [in]  user context
 * @param req [in]      request to be sent
 * @param resp [out]    received response
 * @param timeout [in]  timeout for response wait (in milliseconds)
 *
 * @return MDV_OK if message is successfully sent and 'resp' contains response from remote user
 * @return MDV_BUSY if there is no free slots for request. At this case caller should wait and try again later.
 * @return On error return nonzero error code
 */
mdv_errno mdv_connection_send(mdv_connection *channel, mdv_msg *req, mdv_msg *resp, size_t timeout);


/**
 * @brief Send message but response isn't required.
 *
 * @param channel [in]  user context
 * @param msg [in]      message to be sent
 *
 * @return On success returns MDV_OK
 * @return On error return nonzero error code.
 */
mdv_errno mdv_connection_post(mdv_connection *channel, mdv_msg *msg);


/**
 * @brief Reads incoming data
 *
 * @param channel [in]     user context
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero error code
 */
mdv_errno mdv_connection_recv(mdv_connection *channel);


/**
 * @brief Waits connection ready status
 *
 * @param channel [in]  user context
 * @param timeout [in]  timeout for wait (in milliseconds)
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero error code
 */
mdv_errno mdv_connection_wait(mdv_connection *channel, size_t timeout);
