/**
 * @file
 * @brief File descriptor for event notification.
 * @details The behavior is like a semaphore. Write operation increases the counter stored in eventfd. Read operation clears the value stored in evendfd.
*/

#pragma once
#include "mdv_def.h"


/**
 * @brief   Create a file descriptor for event notification.
 *
 * @return On success, return a new eventfd file descriptor
 * @return On error, return MDV_INVALID_DESCRIPTOR
 */
mdv_descriptor mdv_eventfd();

/**
 * @brief Closes file descriptor for event notification.
 *
 * @param fd [in]   file descriptor for event notification
 */
void mdv_eventfd_close(mdv_descriptor fd);
