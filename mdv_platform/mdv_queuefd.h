/**
 * @file
 * @brief Thread-safe version of queue which support multiple consumers and producers.
 */
#pragma once
#include "mdv_eventfd.h"
#include "mdv_queue.h"
#include "mdv_mutex.h"


/**
 * @brief Base data structure for queuefd. Any queuefd can be casted to this base type.
 */
typedef struct
{
    mdv_mutex      rmutex;      ///< mutex for consumer threads
    mdv_mutex      wmutex;      ///< mutex for producer threads
    mdv_descriptor event;       ///< event for consumer threads notification
    mdv_queue_base queue;       ///< queue
} mdv_queuefd_base;


/**
 * @brief Allocate new queuefd on the stack.
 *
 * @details New created queue is not initialized. Use mdv_queuefd_init() to initialize the queue.
 *
 * @param type [in] Queue items type.
 * @param sz [in]   Queue  capacity.
 */
#define mdv_queuefd(type, sz)               \
    struct                                  \
    {                                       \
        mdv_mutex           rmutex;         \
        mdv_mutex           wmutex;         \
        mdv_descriptor      event;          \
        mdv_queue(type, sz) queue;          \
    }


/// @cond Doxygen_Suppress

int    _mdv_queuefd_init(mdv_queuefd_base *queue);
void   _mdv_queuefd_free(mdv_queuefd_base *queue);
int    _mdv_queuefd_push(mdv_queuefd_base *queue, void const *data, size_t size);
int    _mdv_queuefd_pop(mdv_queuefd_base *queue, void *data, size_t size);

/// @endcond


/**
 * @brief Initialize queuefd.
 *
 * @details Use mdv_queuefd_ok() to check queuefd validity
 *
 * @param q [in] Queue allocated by mdv_queuefd()
 */
#define mdv_queuefd_init(q)                                     \
    if (_mdv_queuefd_init((mdv_queuefd_base *)&(q)))            \
    {                                                           \
        mdv_queue_clear((q).queue);                             \
    }


/**
 * @brief Free resources captured by queuefd.
 *
 * @param q [in] Queue allocated by mdv_queuefd()
 */
#define mdv_queuefd_free(q)                                     \
    _mdv_queuefd_free((mdv_queuefd_base *)&(q))


/**
 * @brief Return items count in queuefd.
 *
 * @param q [in] Queue
 *
 * @return queue size
 */
#define mdv_queuefd_size(q)                   \
    mdv_queue_size((q).queue)


/**
 * @brief Check is queue valid.
 *
 * @param q [in] Queue allocated by mdv_queuefd()
 *
 * @return true if queue is valid
 * @return false if queue is invalid
 */
#define mdv_queuefd_ok(q) ((q).event != MDV_INVALID_DESCRIPTOR)


/// @cond Doxygen_Suppress

#define mdv_queuefd_push_one(q, item)                   \
    _mdv_queuefd_push((mdv_queuefd_base *)&(q), &(item), sizeof(item))


#define mdv_queuefd_pop_one(q, item)                    \
    _mdv_queuefd_pop((mdv_queuefd_base *)&(q), &(item), sizeof(item))


/**
 * @brief Push one or multiple items to the back of queuefd.
 *
 * @param q [in]     queue
 * @param items [in] new items array or single item
 * @param count [in] items array length (optional)
 *
 * @return On success, return nonzero value
 * @return On error, return zero
 */
#define mdv_queuefd_push(...)                                       \
    mdv_queuefd_push_one(__VA_ARGS__)


/**
 * @brief Pop one or multiple items from the front of queuefd.
 *
 * @param q [in]      queue
 * @param items [out] items array or single item
 * @param count [in]  items array length (optional)
 *
 * @return On success, return nonzero value
 * @return On error, return zero
 */
#define mdv_queuefd_pop(...)                                    \
    mdv_queuefd_pop_one(__VA_ARGS__)

/// @endcond

