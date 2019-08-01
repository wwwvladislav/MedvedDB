/**
 * @file mdv_jobber.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief This file contains functionality for jobs scheduling and deferred running.
 * @details Job scheduler work on thread pool.
 * @version 0.1
 * @date 2019-08-01
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov`
 *
 */
#pragma once
#include "mdv_threadpool.h"


/// Jobs scheduler configuration
typedef struct mdv_jobber_config
{
    mdv_threadpool_config   threadpool;     ///< thread pool options
    struct
    {
        size_t  count;                      ///< Job queues count
    } queue;                                ///< Job queue settings
} mdv_jobber_config;


/// Jobs scheduler
typedef struct mdv_jobber mdv_jobber;


/**
 * @brief Create and start new job scheduler
 *
 * @param config [in] job scheduler configuration
 *
 * @return new job scheduler or NULL pointer if error occured
 */
mdv_jobber * mdv_jobber_create(mdv_jobber_config const *config);


/**
 * @brief Stop and free job scheduler
 *
 * @param jobber [in] job scheduler
 */
void mdv_jobber_free(mdv_jobber *jobber);