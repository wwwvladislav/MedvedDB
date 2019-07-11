/**
 * @file
 * @brief File descriptors event handler on thread pool.
 */
#pragma once
#include <stddef.h>
#include "mdv_epoll.h"
#include "mdv_threads.h"


/// Thread pool descriptor
typedef struct mdv_threadpool mdv_threadpool;


/**
 * @brief Thread pool task.
 * @details Thread pool waits (on epoll) and triggers event handler function for some event raised on file descriptor.
 */
typedef struct mdv_threadpool_task_base
{
    mdv_descriptor  fd;                                 ///< file descriptor
    void (*fn)(uint32_t events,
               struct mdv_threadpool_task_base *task);  ///< Event handler function
    size_t          context_size;                       ///< Context size
    char            context[1];                         ///< Context associated with handler
} mdv_threadpool_task_base;


/// Thread pool task definition
#define mdv_threadpool_task(context_type)           \
    struct                                          \
    {                                               \
        mdv_descriptor  fd;                         \
        void (*fn)(uint32_t events,                 \
                   mdv_threadpool_task_base *task); \
        size_t          context_size;               \
        context_type    context;                    \
    }


/// Thread pool configuration. All options are mandatory.
typedef struct mdv_threadpool_config
{
    size_t           size;              ///< threads count in thread pool
    mdv_thread_attrs thread_attrs;      ///< Attributes for a new thread
} mdv_threadpool_config;


/**
 * @brief Create and start new thread pool
 *
 * @param config [in] threads pool configuration
 *
 * @return On success, return pointer to a new created thread pool
 * @return On error, return NULL
 */
mdv_threadpool * mdv_threadpool_create(mdv_threadpool_config const *config);


/**
 * @brief Stop thread pool
 *
 * @param threadpool [in] thread pool
 */
void mdv_threadpool_stop(mdv_threadpool *threadpool);


/**
 * @brief Free thread pool. Use mdv_threadpool_stop() before the freeing the thread pool.
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
 * @return On success, nonzero pointer to new inserted task
 * @return On error, NULL pointer
 */
mdv_threadpool_task_base * mdv_threadpool_add(mdv_threadpool *threadpool, uint32_t events, mdv_threadpool_task_base const *task);


/**
 * @brief Rearm existing task in thread pool.
 *
 * @param threadpool [in] thread pool
 * @param events [in]     Epoll events. It should be the bitwise OR combination of the mdv_epoll_events.
 * @param task [in]       task, already registered in thread pool
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_threadpool_rearm(mdv_threadpool *threadpool, uint32_t events, mdv_threadpool_task_base *task);


/**
 * @brief Remove task associated with file descriptor
 *
 * @param threadpool [in] thread pool
 * @param fd [in]         file descriptor
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_threadpool_remove(mdv_threadpool *threadpool, mdv_descriptor fd);


/**
 * @brief Call function for each file descriptor in thread pool
 *
 * @param threadpool [in] thread pool
 * @param fn [in] file descriptor handler
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_threadpool_foreach(mdv_threadpool *threadpool, void (*fn)(mdv_descriptor fd, void *context));
