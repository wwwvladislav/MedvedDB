/**
 * @file mdv_syncerino.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Transaction logs synchronizer with specific node
 * @version 0.1
 * @date 2019-11-27
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_ebus.h>
#include <mdv_uuid.h>


/// Data synchronizer with specific node
typedef struct mdv_syncerino mdv_syncerino;


/**
 * @brief Creates transaction logs synchronizer with specific node
 *
 * @param uuid [in]         Global unique identifier for node
 * @param ebus [in]         Events bus
 *
 * @return Data committer
 */
mdv_syncerino * mdv_syncerino_create(mdv_uuid const *uuid, mdv_ebus *ebus);


/**
 * @brief Retains data synchronizer.
 * @details Reference counter is increased by one.
 */
mdv_syncerino * mdv_syncerino_retain(mdv_syncerino *syncerino);


/**
 * @brief Releases data synchronizer.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the data synchronizer is stopped and freed.
 */
uint32_t mdv_syncerino_release(mdv_syncerino *syncerino);


/**
 * @brief Cancels all synchronization jobs
 *
 * @param syncerino [in] transaction logs synchronizer with specific node
 */
void mdv_syncerino_cancel(mdv_syncerino *syncerino);


/**
 * @brief Starts transaction log synchronization
 *
 * @param syncerino [in] transaction logs synchronizer with specific node
 * @param trlog [in] transaction log UUID
 */
mdv_errno mdv_syncerino_start(mdv_syncerino *syncerino, mdv_uuid const *trlog);
