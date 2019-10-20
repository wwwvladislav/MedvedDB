/**
 * @file mdv_core.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Core component for cluster nodes management and storage accessing.
 * @version 0.1
 * @date 2019-07-31
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_def.h>


/// Core component for cluster nodes management and storage accessing.
typedef struct mdv_core mdv_core;


/**
 * @brief Create new core.
 */
mdv_core * mdv_core_create();


/**
 * @brief Frees resources used by core.
 *
 * @param core [in] core
 */
void mdv_core_free(mdv_core *core);


/**
 * @brief Listen incomming connections
 *
 * @param core [in] core
 *
 * @return true if listener successfully started
 * @return false if error is occurred
 */
bool mdv_core_listen(mdv_core *core);


/**
 * @brief Connect to other peers
 *
 * @param core [in] core
 */
void mdv_core_connect(mdv_core *core);

