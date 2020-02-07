/**
 * @file mdv_trfd.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief File descriptor based transport
 * @version 0.1
 * @date 2020-02-07
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_transport.h"


/**
 * @brief Create new file descriptor based transport
 *
 * @param fd [in] file descriptor
 *
 * @return On success returns new file descriptor based transport
 * @return On error returns NULL pointer
 */
mdv_transport * mdv_trfd_create(mdv_descriptor fd);
