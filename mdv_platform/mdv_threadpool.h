/**
 * @file
 * @brief File descriptors event handler on thread pool.
 */
#pragma once
#include <stddef.h>
#include "mdv_epoll.h"


/// Thread pool descriptor
typedef struct mdv_threadpool mdv_threadpool;


/**
 * @brief Thread pool task.
 * @details Thread pool waits (on epoll) and triggers event handler function for some event was raised on file descriptor.
 */
typedef struct mdv_threadpool_task
{
    mdv_descriptor  fd;         ///< file descriptor
    void           *data;       ///< User data associated with handler
    void (*fn)(mdv_descriptor fd,
               uint32_t events,
               void *data);     ///< Event handler function
} mdv_threadpool_task;


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


/**
 * @brief Add new task to thread pool
 *
 * @param threadpool [in] thread pool
 * @param task [in]       New task
 * @param events [in]     Epoll events. It should be the bitwise OR combination of the mdv_epoll_events.
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_threadpool_add(mdv_threadpool *threadpool, uint32_t events, mdv_threadpool_task const *task);
