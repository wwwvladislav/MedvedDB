/**
 * @file mdv_service.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief API for MDV service management
 * @version 0.1
 * @date 2020-05-09
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include <stdbool.h>


/// Service
typedef struct mdv_service mdv_service;


/**
 * @brief Main MDV service creation
 *
 * @param cfg_file_path [in] path to configuration file
 *
 * @return On success returns pointer to new created service, otherwise returnc NULL pointer.
 */
mdv_service * mdv_service_create(char const *cfg_file_path);


/**
 * @brief Frees resources allocated by service
 *
 * @param svc [in] MDV service
 */
void mdv_service_free(mdv_service *svc);


/**
 * @brief Starts DMV service.
 *
 * @param svc [in] MDV service
 */
bool mdv_service_start(mdv_service *svc);


/**
 * @brief Waits MDV service completion
 *
 * @param svc [in] MDV service
 */
void mdv_service_wait(mdv_service *svc);


/**
 * @brief Stops MDV service
 *
 * @param svc [in] MDV service
 */
void mdv_service_stop(mdv_service *svc);
