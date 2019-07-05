#include "mdv_threadpool.h"
#include "mdv_queuefd.h"
#include "mdv_eventfd.h"
#include "mdv_threads.h"
#include "mdv_mutex.h"
#include "mdv_alloc.h"
#include "mdv_log.h"
#include "mdv_rollbacker.h"
#include "mdv_hashmap.h"
#include <string.h>


/// @cond Doxygen_Suppress

enum
{
    MDV_TP_QUEUEFD_SIZE = 1024      /// Events queue size per thread in thread pool.
};

/// @endcond


/// Thread pool descriptor
struct mdv_threadpool
{
    size_t          size;           ///< Number of threasds in thread pool
    mdv_descriptor  stopfd;         ///< eventfd for thread pool stop notification
    mdv_descriptor  epollfd;        ///< epoll file descriptor
    mdv_thread     *threads;        ///< threads array
    mdv_mutex      *tasks_mtx;      ///< mutex for tasks protection
    mdv_hashmap     tasks;          ///< file descriptors and handlers
    uint8_t         data_space[1];  ///< data space
};


static void * mdv_threadpool_worker(void *arg)
{
    mdv_threadpool *threadpool = (mdv_threadpool *)arg;

    int work = 1;

    while(work)
    {
        uint32_t size = 64;
        mdv_epoll_event events[size];

        mdv_errno err = mdv_epoll_wait(threadpool->epollfd, events, &size, -1);

        if (err != MDV_OK)
        {
            MDV_LOGE("Worker thread stopped due the error '%s' (%d)", mdv_strerror(err), err);
            break;
        }

        for(uint32_t i = 0; i < size; ++i)
        {
            mdv_threadpool_task *task = (mdv_threadpool_task *)events[i].data;

            if(task->fd == threadpool->stopfd)
            {
                work = 0;
                break;
            }

            if (task->fn)
                task->fn(task->fd, events[i].events, task->data);
            else
                MDV_LOGE("Thread pool task without handler was skipped");
        }
    }

    return 0;
}


static mdv_errno mdv_threadpool_add_default_tasks(mdv_threadpool *threadpool)
{
    mdv_threadpool_task task =
    {
        .fd = threadpool->stopfd
    };

    mdv_errno err = mdv_threadpool_add(threadpool, MDV_EPOLLIN | MDV_EPOLLERR, &task);

    return err;
}


mdv_threadpool * mdv_threadpool_create(size_t size)
{
    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    size_t const mem_size = offsetof(mdv_threadpool, data_space) + size * sizeof(mdv_thread);

    mdv_threadpool *tp = (mdv_threadpool *)mdv_alloc(mem_size);

    if(!tp)
    {
        MDV_LOGE("threadpool_create failed");
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, tp);

    memset(tp->data_space, 0, mem_size - offsetof(mdv_threadpool, data_space));


    tp->size = size;


    tp->epollfd = mdv_epoll_create();

    if (tp->epollfd == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("threadpool_create failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_epoll_close, tp->epollfd);


    tp->stopfd = mdv_eventfd();

    if (tp->stopfd == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("threadpool_create failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_eventfd_close, tp->stopfd);


    tp->tasks_mtx = mdv_mutex_create();

    if (!tp->tasks_mtx)
    {
        MDV_LOGE("threadpool_create failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, tp->tasks_mtx);


    if (!mdv_hashmap_init(tp->tasks, mdv_threadpool_task, fd, 256, mdv_descriptor_hash, mdv_descriptor_cmp))
    {
        MDV_LOGE("threadpool_create failed");
        mdv_rollback(rollbacker);
        return 0;
    }


    if (mdv_threadpool_add_default_tasks(tp) != MDV_OK)
    {
        MDV_LOGE("threadpool_create failed");
        mdv_threadpool_free(tp);
        return 0;
    }


    tp->threads = (mdv_thread*)tp->data_space;

    for(size_t i = 0; i < size; ++i)
    {
        mdv_errno err = mdv_thread_create(tp->threads + i, mdv_threadpool_worker, tp);
        if (err != MDV_OK)
        {
            MDV_LOGE("threadpool_create failed");
            mdv_threadpool_free(tp);
            return 0;
        }
    }

    return tp;
}


static void mdv_threadpool_stop(mdv_threadpool *tp)
{
    uint64_t data = 0xDEADFA11;
    size_t len = sizeof data;

    while(mdv_write(tp->stopfd, &data, &len) == MDV_EAGAIN)
    {
        len = sizeof data;
    }

    for(size_t i = 0; i < tp->size; ++i)
    {
        if (tp->threads[i])
        {
            mdv_errno err = mdv_thread_join(tp->threads[i]);
            if (err != MDV_OK)
                MDV_LOGE("Thread join failed with error '%s' (%d)", mdv_strerror(err), err);
        }
    }
}


void mdv_threadpool_free(mdv_threadpool *tp)
{
    if (tp)
    {
        mdv_threadpool_stop(tp);
        mdv_epoll_close(tp->epollfd);
        mdv_eventfd_close(tp->stopfd);
        mdv_hashmap_free(tp->tasks);
        mdv_mutex_free(tp->tasks_mtx);
        mdv_free(tp);
    }
}


mdv_errno mdv_threadpool_add(mdv_threadpool *threadpool, uint32_t events, mdv_threadpool_task const *task)
{
    mdv_errno err = mdv_mutex_lock(threadpool->tasks_mtx);

    if(err == MDV_OK)
    {
        mdv_list_entry_base *entry = mdv_hashmap_insert(threadpool->tasks, *task);

        if (entry)
        {
            mdv_epoll_event evt = { events, entry->item };

            err = mdv_epoll_add(threadpool->epollfd, task->fd, evt);

            if (err != MDV_OK)
            {
                mdv_hashmap_erase(threadpool->tasks, task->fd);
                MDV_LOGE("Threadpool task registration failed with error '%s' (%d)", mdv_strerror(err), err);
            }
        }
        else
        {
            MDV_LOGE("Threadpool task registration failed");
            err = MDV_FAILED;
        }

        mdv_mutex_unlock(threadpool->tasks_mtx);
    }
    else
        MDV_LOGE("Threadpool task registration failed");

    return err;
}


mdv_errno mdv_threadpool_remove(mdv_threadpool *threadpool, mdv_descriptor fd)
{
    mdv_errno err = mdv_mutex_lock(threadpool->tasks_mtx);

    if(err == MDV_OK)
    {
        err = mdv_epoll_del(threadpool->epollfd, fd);

        if (err == MDV_OK)
        {
            mdv_hashmap_erase(threadpool->tasks, fd);
        }
        else
            MDV_LOGE("Threadpool task removing failed with error '%s' (%d)", mdv_strerror(err), err);

        mdv_mutex_unlock(threadpool->tasks_mtx);
    }
    else
        MDV_LOGE("Threadpool task removing failed");

    return err;
}

