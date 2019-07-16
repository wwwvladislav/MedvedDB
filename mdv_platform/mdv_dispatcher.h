/**
 * @file
 * @brief Messages dispatcher allow to send multiple messages and wait the response.
 */
#pragma once
#include "mdv_msg.h"
#include "mdv_errno.h"


/// Messages dispatcher
typedef struct mdv_dispatcher mdv_dispatcher;


/// Message handler function
typedef mdv_errno (*mdv_dispatcher_handler_fn)(mdv_msg const *msg, void *arg);


/// Message handler
typedef struct mdv_dispatcher_handler
{
    uint16_t                    id;     ///< Message identifier
    mdv_dispatcher_handler_fn   fn;     ///< Message handler
    void                       *arg;    ///< Argument which passed to message handler
} mdv_dispatcher_handler;


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
mdv_dispatcher * mdv_dispatcher_create(mdv_descriptor fd, size_t size, mdv_dispatcher_handler const *handlers);


/**
 * @brief Free messages dispatcher
 *
 * @param pd [in] messages dispatcher
 */
void mdv_dispatcher_free(mdv_dispatcher *pd);


/**
 * @brief Set file descriptor
 *
 * @param pd [in] messages dispatcher
 * @param fd [in] file descriptor for messages dispatching
 */
void mdv_dispatcher_set_fd(mdv_dispatcher *pd, mdv_descriptor fd);


/**
 * @brief Close file descriptor
 *
 * @param pd [in] messages dispatcher
 */
void mdv_dispatcher_close_fd(mdv_dispatcher *pd);


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
mdv_errno mdv_dispatcher_send(mdv_dispatcher *pd, mdv_msg *req, mdv_msg *resp, size_t timeout);


/**
 * @brief Send message but response isn't required.
 *
 * @param pd [in]       messages dispatcher
 * @param msg [in]      message to be sent
 *
 * @return On success returns MDV_OK
 * @return On error return nonzero error code.
 */
mdv_errno mdv_dispatcher_post(mdv_dispatcher *pd, mdv_msg *msg);


/**
 * @brief Write raw data
 *
 * @param pd [in]       messages dispatcher
 * @param data [in]     data
 * @param size [in]     data size
 *
 * @return On success returns MDV_OK
 * @return On error return nonzero error code.
 */
mdv_errno mdv_dispatcher_write_raw(mdv_dispatcher *pd, void const *data, size_t size);


/**
 * @brief Send response
 *
 * @param pd [in]       messages dispatcher
 * @param msg [in]      message to be sent
 *
 * @return On success returns MDV_OK
 * @return On error return nonzero error code.
 */
mdv_errno mdv_dispatcher_reply(mdv_dispatcher *pd, mdv_msg const *msg);


/**
 * @brief External notification for data reading
 *
 * @param pd [in]       messages dispatcher
 *
 * @return On success returns MDV_OK
 * @return On error return nonzero error code
 */
mdv_errno mdv_dispatcher_read(mdv_dispatcher *pd);

