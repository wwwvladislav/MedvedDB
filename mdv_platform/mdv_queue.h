/**
 * @file
 * @brief Queue.
 *
 * @details Queue is implemented as static ring buffer.
 */
#pragma once
#include <string.h>
#include <stdatomic.h>


/**
 * @brief Base data structure for queue. Any queue can be casted to this base type.
 */
typedef struct
{
    size_t              capacity;   ///< queue capacity
    size_t              item_size;  ///< item size
    atomic_int_fast32_t head;       ///< pointer to the head of ring buffer
    atomic_int_fast32_t tail;       ///< pointer to the tail of ring buffer
    char                data[1];    ///< data
} mdv_queue_base;


/// @cond Doxygen_Suppress
int     _mdv_queue_push_one(mdv_queue_base *queue, void const *data, size_t size);
int     _mdv_queue_pop_one(mdv_queue_base *queue, void *data, size_t size);
int     _mdv_queue_push_multiple(mdv_queue_base *queue, void const *data, size_t size);
int     _mdv_queue_pop_multiple(mdv_queue_base *queue, void *data, size_t size);
size_t  _mdv_queue_size(mdv_queue_base const *queue);
size_t  _mdv_queue_free_space(mdv_queue_base const *queue);
int     _mdv_queue_empty(mdv_queue_base const *queue);
/// @endcond


/**
 * @brief Allocate new queue on the stack.
 *
 * @details New created queue is not initialized. Use mdv_queue_clear() to clear and initialize the queue.
 *
 * @param type [in] Queue items type.
 * @param sz [in]   Queue  capacity.
 */
#define mdv_queue(type, sz)                 \
    struct                                  \
    {                                       \
        size_t              capacity;       \
        size_t              item_size;      \
        atomic_int_fast32_t head;           \
        atomic_int_fast32_t tail;           \
        type                data[sz + 1];   \
    }


/**
 * @brief Clear and initialize queue.
 *
 * @param q [in] Queue allocated by mdv_queue()
 */
#define mdv_queue_clear(q)                                      \
    (q).capacity = sizeof((q).data) / sizeof(*(q).data) - 1;    \
    (q).item_size = sizeof(*(q).data);                          \
    atomic_init(&(q).head, 0);                                  \
    atomic_init(&(q).tail, 0);


/**
 * @brief Check queue is empty.
 *
 * @param q [in] Queue
 *
 * @return 0 if queue is empty
 * @return 1 if queue is not empty
 */
#define mdv_queue_empty(q)                  \
    _mdv_queue_empty((mdv_queue_base *)&(q))


/**
 * @brief Return items count in queue.
 *
 * @param q [in] Queue
 *
 * @return queue size
 */
#define mdv_queue_size(q)                   \
    _mdv_queue_size((mdv_queue_base *)&(q))


/**
 * @brief Return queue capacity.
 *
 * @param q [in] Queue
 *
 * @return queue capacity
 */
#define mdv_queue_capacity(q) (q).capacity


/**
 * @brief Return amount of free space in queue.
 *
 * @param q [in] Queue
 *
 * @return amount of free space in queue.
 */
#define mdv_queue_free_space(q)             \
    _mdv_queue_free_space((mdv_queue_base *)&(q))


/// @cond Doxygen_Suppress
#define mdv_queue_push_multiple(q, items, count)    \
    _mdv_queue_push_multiple((mdv_queue_base *)&(q), items, sizeof(items))


#define mdv_queue_pop_multiple(q, items, count)    \
    _mdv_queue_pop_multiple((mdv_queue_base *)&(q), items, sizeof(items))


#define mdv_queue_push_one(q, item)         \
    _mdv_queue_push_one((mdv_queue_base *)&(q), &(item), sizeof(item))


#define mdv_queue_pop_one(q, item)          \
    _mdv_queue_pop_one((mdv_queue_base *)&(q), &(item), sizeof(item))


#define mdv_queue_push_any(q, items, count, M, ...) M


#define mdv_queue_pop_any(q, items, count, M, ...) M


/**
 * @brief Push one or multiple items to the back of queue.
 *
 * @param q [in]     queue
 * @param items [in] new items array or single item
 * @param count [in] items array length (optional)
 *
 * @return On success, return nonzero value
 * @return On error, return zero
 */
#define mdv_queue_push(...)                                     \
    mdv_queue_push_any(__VA_ARGS__,                             \
                       mdv_queue_push_multiple(__VA_ARGS__),    \
                       mdv_queue_push_one(__VA_ARGS__))




/**
 * @brief Pop one or multiple items from the front of queue.
 *
 * @param q [in]      queue
 * @param items [out] items array or single item
 * @param count [in]  items array length (optional)
 *
 * @return On success, return nonzero value
 * @return On error, return zero
 */
#define mdv_queue_pop(...)                                      \
    mdv_queue_pop_any(__VA_ARGS__,                              \
                      mdv_queue_pop_multiple(__VA_ARGS__),      \
                      mdv_queue_pop_one(__VA_ARGS__))
/// @endcond
