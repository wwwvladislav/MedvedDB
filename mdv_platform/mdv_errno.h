/**
 * @file
 * @brief This header contains common error definitions end error type declaration.
 *
*/

#pragma once


/// Common error definitions
enum
{
    MDV_OK          = 0,    ///< Operation successfully completed.
    MDV_FAILED      = -1,   ///< Operation failed due the unknown error.
    MDV_INVALID_ARG = -2,   ///< Function argument is invalid.
    MDV_EAGAIN      = -3,   ///< Resource temporarily unavailable. There is no data available right now, try again later.
    MDV_CLOSED      = -4,   ///< File descriptor closed.
    MDV_EEXIST      = -5,   ///< File exists
    MDV_NO_MEM      = -6,   ///< No free space of memory
    MDV_INPROGRESS  = -7,   ///< Operation now in progress
    MDV_ETIMEDOUT   = -8,   ///< Timed out
    MDV_BUSY        = -9    ///< Resource is busy
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
 *
 * @return string description for error number
 */
char const * mdv_strerror(mdv_errno err);
