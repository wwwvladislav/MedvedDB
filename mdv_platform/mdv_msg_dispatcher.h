/**
 * @file
 * @brief Messages dispatcher allow to send multiple messages and wait the response.
 */
#pragma once
#include "mdv_msg.h"
#include "mdv_errno.h"


/// Messages dispatcher
typedef struct mdv_msg_dispatcher mdv_msg_dispatcher;


/// Message handler function
typedef mdv_errno (*mdv_msg_handler_fn)(mdv_msg const *msg, void *arg);


/// Message handler
typedef struct mdv_msg_handler
{
    uint16_t             id;
    mdv_msg_handler_fn   fn;
    void                *arg;
} mdv_msg_handler;


/**
 * @brief Create new messages dispatcher
 *
 * @param fd [in]       file descriptor for messages dispatching
 * @param size [in]     message handlers count
 * @param handlers [in] message handlers
 *
 * @return On success return new messages dispatcher.
 * @return On error return NULL pointer
 */
mdv_msg_dispatcher * mdv_msg_dispatcher_create(mdv_descriptor fd, size_t size, mdv_msg_handler const *handlers);


/**
 * @breief Free messages dispatcher
 *
 * @param pd [in] messages dispatcher
 */
void mdv_msg_dispatcher_free(mdv_msg_dispatcher *pd);


/**
 * @brief Send message and wait response.
 *
 * @param pd [in]       messages dispatcher
 * @param req [in]      request to be sent
 * @param resp [out]    received response
 * @param timeout [in]  timeout for response wait (in milliseconds)
 *
 * @return MDV_OK if message is successfully sent and 'resp' contains response from remote peer
 * @return MDV_BUSY if there is no free slots for request. At this case caller should wait and try again later.
 * @return On error return nonzero error code
 */
mdv_errno mdv_msg_dispatcher_send(mdv_msg_dispatcher *pd, mdv_msg const *req, mdv_msg *resp, size_t timeout);


/**
 * @brief Send message but response isn't required.
 *
 * @param pd [in]       messages dispatcher
 * @param msg [in]      message to be sent
 *
 * @return On success returns MDV_OK
 * @return On error return nonzero error code.
 */
mdv_errno mdv_msg_dispatcher_post(mdv_msg_dispatcher *pd, mdv_msg const *msg);



/**
 * @brief External notification for data reading
 *
 * @param pd [in]       messages dispatcher
 * @param msg [out]     received message
 *
 * @return On success returns MDV_OK
 * @return On error return nonzero error code
 */
mdv_errno mdv_msg_dispatcher_read(mdv_msg_dispatcher *pd);
