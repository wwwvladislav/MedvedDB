#include "mdv_queuefd.h"
#include "mdv_rollbacker.h"
#include "mdv_log.h"
#include "mdv_def.h"


int _mdv_queuefd_init(mdv_queuefd_base *queue)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    if (mdv_mutex_create(&queue->rmutex) != MDV_OK)
    {
        MDV_LOGE("queuefd_init failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &queue->rmutex);

    if (mdv_mutex_create(&queue->wmutex) != MDV_OK)
    {
        MDV_LOGE("queuefd_init failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &queue->wmutex);

    queue->event = mdv_eventfd(true);

    if (queue->event == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("queuefd_init failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_free(rollbacker);

    return 1;
}


void _mdv_queuefd_free(mdv_queuefd_base *queue)
{
    if (queue)
    {
        mdv_eventfd_close(queue->event);
        queue->event = MDV_INVALID_DESCRIPTOR;
        mdv_mutex_free(&queue->rmutex);
        mdv_mutex_free(&queue->wmutex);
    }
}


int _mdv_queuefd_push(mdv_queuefd_base *queue, void const *data, size_t size)
{
    int ret = 0;

    if (mdv_mutex_lock(&queue->wmutex) == MDV_OK)
    {
        if (_mdv_queue_push_one(&queue->queue, data, size))
        {
            uint64_t n = 1;
            size_t len = sizeof n;
            ret = mdv_write(queue->event, &n, &len) == MDV_OK && len == sizeof n;
        }
        mdv_mutex_unlock(&queue->wmutex);
    }

    return ret;
}


int _mdv_queuefd_pop(mdv_queuefd_base *queue, void *data, size_t size)
{
    int ret = 0;

    if (mdv_mutex_lock(&queue->rmutex) == MDV_OK)
    {
        uint64_t n;
        size_t len = sizeof n;

        if (mdv_read(queue->event, &n, &len) == MDV_OK && len == sizeof n)
        {
            ret = _mdv_queue_pop_one(&queue->queue, data, size);
        }

        mdv_mutex_unlock(&queue->rmutex);
    }

    return ret;
}

