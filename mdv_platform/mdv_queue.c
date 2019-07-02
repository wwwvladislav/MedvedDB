#include "mdv_queue.h"


int _mdv_queue_push_one(mdv_queue_base *queue, void const *data, size_t size)
{
    if (size != queue->item_size)
        return 0;

    size_t const head = atomic_load_explicit(&queue->head, memory_order_relaxed);
    size_t const tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    size_t const capacity = queue->capacity + 1;
    size_t const data_size = head <= tail
                                ? tail - head
                                : capacity + tail - head;
    size_t const free_space = queue->capacity - data_size;

    if (!free_space)
        return 0;

    memcpy(queue->data + tail * queue->item_size, data, size);

    atomic_store_explicit(&queue->tail, (tail + 1) % capacity, memory_order_relaxed);

    return 1;
}


int _mdv_queue_push_multiple(mdv_queue_base *queue, void const *data, size_t size)
{
    size_t const items_count = size / queue->item_size;

    if (size != items_count * queue->item_size)
        return 0;

    size_t const head = atomic_load_explicit(&queue->head, memory_order_relaxed);
    size_t const tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    size_t const capacity = queue->capacity + 1;

    if (head <= tail)
    {
        size_t const data_size = tail - head;
        size_t const free_space = queue->capacity - data_size;

        if (free_space < items_count)
            return 0;

        size_t const free_space_0 = capacity - tail;

        if (free_space_0 >= items_count)
            memcpy(queue->data + tail * queue->item_size, data, size);
        else
        {
            size_t const first_block_size = free_space_0 * queue->item_size;
            memcpy(queue->data + tail * queue->item_size, data, first_block_size);
            memcpy(queue->data, data + first_block_size, size - first_block_size);
        }
    }
    else
    {
        size_t const data_size = capacity + tail - head;
        size_t const free_space = queue->capacity - data_size;

        if (free_space < items_count)
            return 0;

        memcpy(queue->data + tail * queue->item_size, data, size);
    }

    atomic_store_explicit(&queue->tail, (tail + items_count) % capacity, memory_order_relaxed);

    return 1;
}


int _mdv_queue_pop_one(mdv_queue_base *queue, void *data, size_t size)
{
    if (size != queue->item_size)
        return 0;

    size_t const head = atomic_load_explicit(&queue->head, memory_order_relaxed);
    size_t const tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    size_t const capacity = queue->capacity + 1;
    size_t const data_size = head <= tail
                                ? tail - head
                                : capacity + tail - head;

    if (!data_size)
        return 0;

    memcpy(data, queue->data + head * queue->item_size, size);

    atomic_store_explicit(&queue->head, (head + 1) % capacity, memory_order_relaxed);

    return 1;
}


int _mdv_queue_pop_multiple(mdv_queue_base *queue, void *data, size_t size)
{
    size_t const items_count = size / queue->item_size;

    if (size != items_count * queue->item_size)
        return 0;

    size_t const head = atomic_load_explicit(&queue->head, memory_order_relaxed);
    size_t const tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    size_t const capacity = queue->capacity + 1;

    if (head <= tail)
    {
        size_t const data_size = tail - head;

        if (items_count > data_size)
            return 0;

        memcpy(data, queue->data + head * queue->item_size, size);
    }
    else
    {
        size_t const data_size = capacity + tail - head;

        if (items_count > data_size)
            return 0;

        size_t const data_block_0 = capacity - head;

        if (data_block_0 >= items_count)
            memcpy(data, queue->data + head * queue->item_size, size);
        else
        {
            size_t const first_block_size = data_block_0 * queue->item_size;
            memcpy(data, queue->data + head * queue->item_size, first_block_size);
            memcpy(data + first_block_size, queue->data, size - first_block_size);
        }
    }

    atomic_store_explicit(&queue->head, (head + items_count) % capacity, memory_order_relaxed);

    return 1;
}


size_t _mdv_queue_size(mdv_queue_base const *queue)
{
    size_t const head = atomic_load_explicit(&queue->head, memory_order_relaxed);
    size_t const tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    size_t const capacity = queue->capacity + 1;

    return head <= tail
            ? tail - head
            : capacity + tail - head;
}


size_t _mdv_queue_free_space(mdv_queue_base const *queue)
{
    return queue->capacity - _mdv_queue_size(queue);
}


int _mdv_queue_empty(mdv_queue_base const *queue)
{
    size_t const head = atomic_load_explicit(&queue->head, memory_order_relaxed);
    size_t const tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
    return head == tail;
}
