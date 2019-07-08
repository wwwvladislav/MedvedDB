/**
 * @file
 * @brief Timer that delivers timer expiration notifications via a file descriptor
*/

#pragma once
#include "mdv_def.h"


/**
 * @brief   Create a new timer.
 *
 * @return On success, return a new timerfd file descriptor
 * @return On error, return MDV_INVALID_DESCRIPTOR
 */
mdv_descriptor mdv_timerfd();


/**
 * @brief Closes file descriptor associated with timer.
 *
 * @param fd [in]   file descriptor for timer
 */
void mdv_timerfd_close(mdv_descriptor fd);


/**
 * @brief  Arms (starts) or disarms (stops) the timer referred to by the file descriptor fd.
 *
 * @param fd [in]       file descriptor for timer
 * @param start [in]    Initial timer expiration in milliseconds
 * @param interval [in] Interval for periodic timer in milliseconds
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
mdv_errno mdv_timerfd_settime(mdv_descriptor fd, size_t start, size_t interval);


/**
 * @brief Return system-wide clock time (i.e., wall-clock time)
 */
size_t mdv_gettime();
