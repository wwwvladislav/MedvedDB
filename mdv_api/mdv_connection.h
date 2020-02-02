/**
 * @file
 * @brief Users connections processing logic.
 */
#pragma once
#include <mdv_def.h>
#include <mdv_msg.h>
#include <mdv_uuid.h>
#include <mdv_channel.h>


/// User connection context used for storing different type of information
/// about connection
typedef struct mdv_connection mdv_connection;


/**
 * @brief Initialize user
 *
 * @param fd [in]       channel descriptor
 * @param uuid [in]     remote node uuid
 *
 * @return On success, return pointer to new user connection context
 * @return On error, return NULL pointer
 */
mdv_connection * mdv_connection_create(mdv_descriptor fd, mdv_uuid const *uuid);


/**
 * @brief Retains user connection context.
 * @details Reference counter is increased by one.
 */
mdv_connection * mdv_connection_retain(mdv_connection *con);


/**
 * @brief Retains user connection context.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the  user's connection context destructor is called.
 */
uint32_t mdv_connection_release(mdv_connection *con);


/**
 * @brief Returns channel UUID
 */
mdv_uuid const * mdv_connection_uuid(mdv_connection *con);


/**
 * @brief Send message and wait response.
 *
 * @param con [in]      user connection context
 * @param req [in]      request to be sent
 * @param resp [out]    received response
 * @param timeout [in]  timeout for response wait (in milliseconds)
 *
 * @return MDV_OK if message is successfully sent and 'resp' contains response from remote user
 * @return MDV_BUSY if there is no free slots for request. At this case caller should wait and try again later.
 * @return On error return nonzero error code
 */
mdv_errno mdv_connection_send(mdv_connection *con, mdv_msg *req, mdv_msg *resp, size_t timeout);


/**
 * @brief Send message but response isn't required.
 *
 * @param con [in]      user connection context
 * @param msg [in]      message to be sent
 *
 * @return On success returns MDV_OK
 * @return On error return nonzero error code.
 */
mdv_errno mdv_connection_post(mdv_connection *con, mdv_msg *msg);
