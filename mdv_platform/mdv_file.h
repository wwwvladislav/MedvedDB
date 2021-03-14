/**
 * @file mdv_file.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Functionality for file input/output
 * @version 0.1
 * @date 2020-04-18
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_def.h"


/**
 * @brief File opening modes
 */
enum mdv_oflags
{
    MDV_OREAD       = 1 << 0,   ///< Open file for read
    MDV_OWRITE      = 1 << 1,   ///< Open file for write
    MDV_OCREAT      = 1 << 2,   ///< If pathname does not exist, create it as a regular file.
    MDV_ODIRECT     = 1 << 3,   ///< Try to minimize cache effects of the I/O to and from this file.
    MDV_ODSYNC      = 1 << 4,   ///< Synchronized  I/O operations
    MDV_OLARGEFILE  = 1 << 5,   ///< (LFS)  Allow files whose sizes cannot be represented
                                ///< in an off_t (but can be represented in an off64_t) to be opened.
    MDV_ONOATIME    = 1 << 6,   ///< Do not update the file last access time (st_atime in the inode) when the file is read.
    MDV_OTMPFILE    = 1 << 7,   ///< Create  an  unnamed temporary regular file.
};


/**
 * @brief Open or possibly create a file.
 *
 * @param pathname [in] file path to be opened
 * @param flags [in]    opening mode (bitvise combination of mdv_oflags)
 *
 * @return On success, return an opened file descriptor
 * @return On error, return MDV_INVALID_DESCRIPTOR
 */
mdv_descriptor mdv_open(const char *pathname, int flags);


/**
 * @brief  Return the file size (in bytes)
 *
 * @param path [in]     file path
 * @param size [out]    pointer where file size is placed
 *
 * @return On success, returns MDV_OK, otherwise error code is returned
 */
mdv_errno mdv_file_size_by_path(const char *path, size_t *size);


/**
 * @brief  Return the file size (in bytes)
 *
 * @param fd [in]       file descriptor
 * @param size [out]    pointer where file size is placed
 *
 * @return On success, returns MDV_OK, otherwise error code is returned
 */
mdv_errno mdv_file_size_by_fd(mdv_descriptor fd, size_t *size);
