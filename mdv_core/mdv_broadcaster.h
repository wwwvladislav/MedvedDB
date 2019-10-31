/**
 * @file mdv_broadcaster.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Gossip protocol implementation for message broadcasting.
 * @version 0.1
 * @date 2019-10-30
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_ebus.h>


///< Message broadcaster
typedef struct mdv_broadcaster mdv_broadcaster;


/**
 * @brief Creates new message broadcaster
 *
 * @param ebus [in]     Events bus
 *
 * @return On success, return non-null pointer to message broadcaster
 * @return On error, return NULL
 */
mdv_broadcaster * mdv_broadcaster_create(mdv_ebus *ebus);


/**
 * @brief Retains message broadcaster.
 * @details Reference counter is increased by one.
 */
mdv_broadcaster * mdv_broadcaster_retain(mdv_broadcaster *broadcaster);


/**
 * @brief Releases message broadcaster.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the message broadcaster is freed.
 */
uint32_t mdv_broadcaster_release(mdv_broadcaster *broadcaster);
