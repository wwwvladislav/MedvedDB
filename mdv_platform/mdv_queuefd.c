#include "mdv_queuefd.h"
#include "mdv_rollbacker.h"
#include "mdv_log.h"
#include <stdint.h>


int _mdv_queuefd_init(mdv_queuefd_base *queue)
{
    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    queue->rmutex = mdv_mutex_create();
    if (!queue->rmutex)
    {
        MDV_LOGE("queuefd_init failed");
        return 0;
    }
    mdv_rollbacker_push(rollbacker, mdv_mutex_free, queue->rmutex);

    queue->wmutex = mdv_mutex_create();
    if (!queue->wmutex)
    {
        MDV_LOGE("queuefd_init failed");
        mdv_rollback(rollbacker);
        return 0;
    }
    mdv_rollbacker_push(rollbacker, mdv_mutex_free, queue->wmutex);

    queue->event = mdv_eventfd();
    if (queue->event == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("queuefd_init failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    return 1;
}


void _mdv_queuefd_free(mdv_queuefd_base *queue)
{
    if (queue)
    {
        mdv_eventfd_close(queue->event);
        queue->event = MDV_INVALID_DESCRIPTOR;
        mdv_mutex_free(queue->rmutex);
        queue->rmutex = 0;
        mdv_mutex_free(queue->wmutex);
        queue->wmutex = 0;
    }
}


int _mdv_queuefd_push_one(mdv_queuefd_base *queue, void const *data, size_t size)
{
    int ret = 0;

    if (mdv_mutex_lock(queue->wmutex) == MDV_OK)
    {
        if (_mdv_queue_push_one(&queue->queue, data, size))
        {
            uint64_t n = 1;
            size_t len = sizeof n;
            ret = mdv_write(queue->event, &n, &len) == MDV_OK && len == sizeof n;
        }
        mdv_mutex_unlock(queue->wmutex);
    }

    return ret;
}


int _mdv_queuefd_pop_one(mdv_queuefd_base *queue, void *data, size_t size)
{
    int ret = 0;

    if (mdv_mutex_lock(queue->rmutex) == MDV_OK)
    {
        ret = _mdv_queue_pop_one(&queue->queue, data, size);
        mdv_mutex_unlock(queue->rmutex);
    }

    return ret;
}


int _mdv_queuefd_push_multiple(mdv_queuefd_base *queue, void const *data, size_t size)
{
    int ret = 0;

    if (mdv_mutex_lock(queue->wmutex) == MDV_OK)
    {
        if (_mdv_queue_push_multiple(&queue->queue, data, size))
        {
            uint64_t n = size / queue->queue.item_size;
            size_t len = sizeof n;
            ret = mdv_write(queue->event, &n, &len) == MDV_OK && len == sizeof n;
        }
        mdv_mutex_unlock(queue->wmutex);
    }

    return ret;
}


int _mdv_queuefd_pop_multiple(mdv_queuefd_base *queue, void *data, size_t size)
{
    int ret = 0;

    if (mdv_mutex_lock(queue->wmutex) == MDV_OK)
    {
        ret = _mdv_queue_pop_multiple(&queue->queue, data, size);
        mdv_mutex_unlock(queue->wmutex);
    }

    return ret;
}


size_t _mdv_queuefd_events_count(mdv_queuefd_base *queue)
{
    uint64_t n = 0;
    size_t len = sizeof n;
    return mdv_read(queue->event, &n, &len) == MDV_OK && len == sizeof n
            ? n
            : 0;
}

