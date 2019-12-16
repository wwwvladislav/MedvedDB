/**
 * @file mdv_fetcher.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Module for data fetching from a table.
 * @version 0.1
 * @date 2019-12-16
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_ebus.h>
#include <mdv_jobber.h>


/// Data fetcher
typedef struct mdv_fetcher mdv_fetcher;


/**
 * @brief Creates data fetcher
 *
 * @param ebus [in]         Events bus
 * @param jconfig [in]      Jobs scheduler configuration
 *
 * @return Data fetcher
 */
mdv_fetcher * mdv_fetcher_create(mdv_ebus *ebus, mdv_jobber_config const *jconfig);


/**
 * @brief Retains data fetcher.
 * @details Reference counter is increased by one.
 */
mdv_fetcher * mdv_fetcher_retain(mdv_fetcher *fetcher);


/**
 * @brief Releases data fetcher.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the data fetcher is stopped and freed.
 */
uint32_t mdv_fetcher_release(mdv_fetcher *fetcher);
