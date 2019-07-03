/**
 * @file
 * @brief File descriptors event handler on thread pool.
 */
#pragma once
#include <stddef.h>


/// Thread pool descriptor
typedef struct mdv_threadpool mdv_threadpool;


/**
 * @brief Create and start new thread pool
 *
 * @param size [in] threads count in pool
 *
 * @return On success, return pointer to a new created thread pool
 * @return On error, return NULL
 */
mdv_threadpool * mdv_threadpool_create(size_t size);


/**
 * @brief Stop and free thread pool
 *
 * @param threadpool [in] thread pool
 */
void mdv_threadpool_free(mdv_threadpool *threadpool);

