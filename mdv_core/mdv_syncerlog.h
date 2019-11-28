/**
 * @file mdv_syncerlog.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Transaction logs synchronizer with specific peer
 * @version 0.1
 * @date 2019-11-28
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_ebus.h>
#include <mdv_uuid.h>
#include <mdv_jobber.h>


/// Data synchronizer with specific peer
typedef struct mdv_syncerlog mdv_syncerlog;


/**
 * @brief Creates transaction logs synchronizer with specific peer
 *
 * @param uuid [in]         Current node UUID
 * @param peer [in]         Global unique identifier for peer
 * @param trlog [in]        Global unique identifier for transaction log
 * @param ebus [in]         Events bus
 * @param jobber [in]       Jobs scheduler
 *
 * @return Data committer
 */
mdv_syncerlog * mdv_syncerlog_create(mdv_uuid const *uuid, mdv_uuid const *peer, mdv_uuid const *trlog, mdv_ebus *ebus, mdv_jobber *jobber);


/**
 * @brief Retains data synchronizer.
 * @details Reference counter is increased by one.
 */
mdv_syncerlog * mdv_syncerlog_retain(mdv_syncerlog *syncerlog);


/**
 * @brief Releases data synchronizer.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the data synchronizer is stopped and freed.
 */
uint32_t mdv_syncerlog_release(mdv_syncerlog *syncerlog);


/**
 * @brief Cancels all synchronization jobs
 *
 * @param syncerlog [in] transaction logs synchronizer with specific node
 */
void mdv_syncerlog_cancel(mdv_syncerlog *syncerlog);
