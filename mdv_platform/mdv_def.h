/**
 * @file
 * @brief Common definitions.
 *
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

