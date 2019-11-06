/**
 * @file
 * @brief Users connections processing logic.
 */
#pragma once
#include <mdv_def.h>
#include <mdv_chaman.h>
#include <mdv_msg.h>
#include <mdv_ebus.h>
#include <mdv_topology.h>


/// User context used for storing different type of information
/// about connection (it should be cast to mdv_conctx)
typedef struct mdv_user mdv_user;


/**
 * @brief Creates user connection context
 *
 * @param fd [in]       channel descriptor
 * @param ebus [in]     events bus
 * @param topology [in] current topology
 *
 * @return On success, return pointer to new user connection context
 * @return On error, return NULL pointer
 */
mdv_user * mdv_user_create(mdv_descriptor fd, mdv_ebus *ebus, mdv_topology *topology);


/**
 * @brief Retains user connection context.
 * @details Reference counter is increased by one.
 */
mdv_user * mdv_user_retain(mdv_user *user);


/**
 * @brief Retains user connection context.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the  user's connection context destructor is called.
 */
uint32_t mdv_user_release(mdv_user *user);


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
 * @brief Reads incomming messages
 *
 * @param user [in]     user context
 *
 * @return On success returns MDV_OK
 * @return On error return nonzero error code.
 */
mdv_errno mdv_user_recv(mdv_user *user);


/**
 * @brief Returns user channel descriptor
 *
 * @param user [in]     user context
 */
mdv_descriptor mdv_user_fd(mdv_user *user);
