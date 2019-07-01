/**
 * @file
 * @brief This header contains common error definitions end error type declaration.
 *
*/

#pragma once


/// Common error definitions
enum
{
    MDV_OK = 0,         ///< Operation was successfully completed.
    MDV_FAILED,         ///< Operation failed due the unknown error.
    MDV_INVALID_ARG,    ///< Function argument is invalid.
    MDV_EAGAIN,         ///< Resource temporarily unavailable. There is no data available right now, try again later.
    MDV_CLOSED,         ///< File descriptor was closed.
    MDV_EEXIST          ///< File exists
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
