/**
 * @file
 * @brief This header contains common error definitions end error type declaration.
 *
*/

#pragma once
#include <stddef.h>


/// Common error definitions
enum
{
    MDV_OK              = 0,    ///< Operation successfully completed.
    MDV_FAILED          = -1,   ///< Operation failed due the unknown error.
    MDV_INVALID_ARG     = -2,   ///< Function argument is invalid.
    MDV_INVALID_TYPE    = -3,   ///< Invalid type.
    MDV_EAGAIN          = -4,   ///< Resource temporarily unavailable. There is no data available right now, try again later.
    MDV_CLOSED          = -5,   ///< File descriptor closed.
    MDV_EEXIST          = -6,   ///< File exists
    MDV_NO_MEM          = -7,   ///< No free space of memory
    MDV_INPROGRESS      = -8,   ///< Operation now in progress
    MDV_ETIMEDOUT       = -9,   ///< Timed out
    MDV_BUSY            = -10,   ///< Resource is busy
    MDV_NO_IMPL         = -11,  ///< No implementaion
    MDV_STACK_OVERFLOW  = -12,  ///< Stack overflow
};


/// Error type declaration
typedef int mdv_errno;


/**
  * @return number of last error
 */
mdv_errno mdv_error();


/**
 * @brief return string description for error number
 *
 * @param err [in]  error number
 * @param buf [out] desination buffer for error message
 * @param size [in] desination buffer size
 *
 * @return string description for error number
 */
char const * mdv_strerror(mdv_errno err, char *buf, size_t size);
