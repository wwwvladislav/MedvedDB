/**
 * @file
 * @brief Common definitions.
 * @details This file contains macros for platform detection, error numbers definitions and file descriptor definition.
*/

#pragma once
#include "mdv_platform.h"
#include "mdv_errno.h"
#include <stddef.h>


/**
 * @brief File descriptor definition can point to file, socket, pipe or epoll instance.
 */
typedef void* mdv_descriptor;


/// Predefined descriptors.
enum mdv_descriptors
{
    MDV_INVALID_DESCRIPTOR = 0      ///< Invalid file descriptor.
};


/**
 * @brief Function for hash calculation for file descriptor
 *
 * @param fd [in]   file descriptor
 *
 * @return hash for file descriptor.
 */
size_t mdv_descriptor_hash(mdv_descriptor const *fd);


/**
 * @brief Function for file descriptors comparison
 *
 * @param fd1 [in]   first file descriptor
 * @param fd2 [in]   second file descriptor
 *
 * @return 0 if file descriptors are equal
 * @return > 0 if first file descriptor is greater then second file descriptor
 * @return < 0 if first file descriptor is less then second file descriptor
 */
int mdv_descriptor_cmp(mdv_descriptor const *fd1, mdv_descriptor const *fd2);


/**
 * @brief Function for writing to a file descriptor.
 *
 * @param fd [in]   file descriptor
 * @param data [in] pointer to the buffer with data
 * @param len [in]  data buffer size in bytes \n
 *            [out] number of bytes which has been written
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_write(mdv_descriptor fd, void const *data, size_t *len);


/**
 * @brief Function for reading from the file descriptor.
 *
 * @param fd [in]   file descriptor
 * @param data [in] pointer to the buffer with data
 * @param len [in]  data buffer size in bytes \n
 *            [out] number of bytes which has been read
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_read(mdv_descriptor fd, void *data, size_t *len);

