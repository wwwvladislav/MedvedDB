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
    MDV_OCREAT      = 1 << 0,   ///< If pathname does not exist, create it as a regular file.
    MDV_ODIRECT     = 1 << 1,   ///< Try to minimize cache effects of the I/O to and from this file.
    MDV_ODSYNC      = 1 << 2,   ///< Synchronized  I/O operations
    MDV_OLARGEFILE  = 1 << 3,   ///< (LFS)  Allow files whose sizes cannot be represented
                                ///< in an off_t (but can be represented in an off64_t) to be opened.
    MDV_ONOATIME    = 1 << 4,   ///< Do not update the file last access time (st_atime in the inode) when the file is read.
    MDV_OTMPFILE    = 1 << 5,   ///< Create  an  unnamed temporary regular file.
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
