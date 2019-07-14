/**
 * @file
 * @brief Messages dispatcher allow to send multiple messages and wait the response.
 */
#pragma once
#include "mdv_msg.h"


/// Messages dispatcher
typedef struct mdv_msg_dispatcher mdv_msg_dispatcher;


/**
 * @brief Create new messages dispatcher
 *
 * @param size [in] maximum number of requests that require an answer
 *
 * @return On success return new messages dispatcher.
 * @return On error return NULL pointer
 */
mdv_msg_dispatcher * mdv_msg_dispatcher_create(size_t size);


/**
 * @breief Free messages dispatcher
 *
 * @param pd [in] messages dispatcher
 */
void mdv_msg_dispatcher_free(mdv_msg_dispatcher *pd);

