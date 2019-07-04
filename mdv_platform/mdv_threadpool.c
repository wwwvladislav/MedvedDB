#include "mdv_threadpool.h"
#include "mdv_queuefd.h"
#include "mdv_eventfd.h"
#include "mdv_threads.h"
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
    uint8_t         data_space[1];  ///< data space
};


static void * mdv_threadpool_worker(void *arg)
{
    while(1)
    {
    }

    return 0;
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

    while(mdv_write(tp->stopfd, &data, &len) == MDV_EAGAIN);

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
        mdv_free(tp);
    }
}
