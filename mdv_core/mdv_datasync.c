#include "mdv_datasync.h"
#include <mdv_alloc.h>
#include <mdv_errno.h>
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <mdv_epoll.h>


typedef struct mdv_datasync_context
{
    uint32_t        peer_dst;       ///< Destination peer identifier
    mdv_datasync   *datasync;       ///< Data synchronizer
    mdv_cfstorage  *storage;        ///< Data storage for synchronization
} mdv_datasync_context;


typedef mdv_job(mdv_datasync_context)     mdv_datasync_job;


static bool mdv_datasync_is_active(mdv_datasync *datasync)
{
    return atomic_load(&datasync->active);
}


static bool mdv_datasync_cfs(void *arg, uint32_t peer_src, uint32_t peer_dst, size_t count, mdv_list const *ops)  // mdv_cfstorage_op ops[]
{
    MDV_LOGE("OOOOOOOOOOOOOOOOOOOOO sync!");

    mdv_list_foreach(ops, mdv_cfstorage_op, op)
    {
        MDV_LOGE("OOOOOOOOOOOOOOOOOOOOO[%llu] size = %u", (unsigned long long)op->row_id, op->op.size);
    }

    // TODO

    return true;
}


static void mdv_datasync_fn(mdv_job_base *job)
{
    mdv_datasync_context *ctx      = (mdv_datasync_context *)job->data;
    mdv_datasync         *datasync = ctx->datasync;
    mdv_cfstorage        *storage  = ctx->storage;

    if (!mdv_datasync_is_active(datasync))
        return;

    // Request sync positions
    // TODO

    // Start synchronization
    if (!mdv_cfstorage_sync(storage, 0, MDV_LOCAL_ID, ctx->peer_dst, 0, mdv_datasync_cfs))
        MDV_LOGE("Data synchronization failed for peer %u", ctx->peer_dst);
}


static void mdv_datasync_finalize(mdv_job_base *job)
{
    mdv_datasync_context *ctx = (mdv_datasync_context *)job->data;
    mdv_free(job, "datasync_job");
}


static bool mdv_datasync_routes(mdv_datasync *datasync, mdv_routes *routes)
{
    bool ret = false;

    if (mdv_mutex_lock(&datasync->mutex) == MDV_OK)
    {
        if (!mdv_vector_empty(datasync->routes))
        {
            if (mdv_vector_create(*routes, mdv_vector_size(datasync->routes), mdv_stallocator))
            {
                mdv_vector_foreach(datasync->routes, uint32_t, route)
                    mdv_vector_push_back(*routes, *route);
                ret = true;
            }
            else
                MDV_LOGE("No memorty for routes. Data synchronization failed.");
        }

        mdv_mutex_unlock(&datasync->mutex);
    }

    return ret;
}


static void mdv_datasync_job_emit(mdv_datasync *datasync, uint32_t peer_dst, mdv_cfstorage *storage)
{
    mdv_datasync_job *job = mdv_alloc(sizeof(mdv_datasync_job), "datasync_job");

    if (!job)
    {
        MDV_LOGE("No memory for data synchronization job");
        return;
    }

    job->fn             = mdv_datasync_fn;
    job->finalize       = mdv_datasync_finalize;
    job->data.peer_dst  = peer_dst;
    job->data.datasync  = datasync;
    job->data.storage   = storage;

    mdv_errno err = mdv_jobber_push(datasync->jobber, (mdv_job_base*)job);

    if (err != MDV_OK)
    {
        MDV_LOGE("Data synchronization job failed");
        mdv_free(job, "datasync_job");
    }
}


// Main data synchronization thread generates asynchronous jobs for each peer.
static void mdv_datasync_main(mdv_datasync *datasync)
{
    mdv_routes routes;

    if (!mdv_datasync_routes(datasync, &routes))
        return;

    mdv_vector_foreach(routes, uint32_t, route)
    {
        mdv_datasync_job_emit(datasync, *route, datasync->tablespace->tables);

        // TODO: push jobs for rows
    }

    mdv_vector_free(routes);
}


static void * mdv_datasync_thread(void *arg)
{
    mdv_datasync *datasync = arg;

    atomic_store(&datasync->active, true);

    mdv_descriptor epfd = mdv_epoll_create();

    if (epfd == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("epoll creation failed");
        return 0;
    }

    mdv_epoll_event const event =
    {
        .events = MDV_EPOLLIN | MDV_EPOLLERR,
        .data = 0
    };

    if (mdv_epoll_add(epfd, datasync->start, event) != MDV_OK)
    {
        MDV_LOGE("epoll event registration failed");
        mdv_epoll_close(epfd);
        return 0;
    }

    while(mdv_datasync_is_active(datasync))
    {
        mdv_epoll_event events[1];
        uint32_t size = sizeof events / sizeof *events;

        mdv_epoll_wait(epfd, events, &size, -1);

        if (!mdv_datasync_is_active(datasync))
            break;

        if (size)
        {
            if (events[0].events & MDV_EPOLLIN)
            {
                uint64_t signal;
                size_t len = sizeof(signal);

                while(mdv_read(datasync->start, &signal, &len) == MDV_EAGAIN);

                // Synchronization start
                mdv_datasync_main(datasync);
            }
        }
    }

    mdv_epoll_close(epfd);

    return 0;
}


void mdv_datasync_stop(mdv_datasync *datasync)
{
    if (mdv_datasync_is_active(datasync))
    {
        atomic_store(&datasync->active, false);

        uint64_t const signal = 1;
        mdv_write_all(datasync->start, &signal, sizeof signal);

        mdv_thread_join(datasync->thread);
    }
}


mdv_errno mdv_datasync_create(mdv_datasync *datasync,
                              mdv_tablespace *tablespace,
                              mdv_tracker *tracker,
                              mdv_jobber *jobber)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    datasync->tablespace = tablespace;
    datasync->tracker = tracker;
    datasync->jobber = jobber;

    mdv_errno err = mdv_mutex_create(&datasync->mutex);

    if (err != MDV_OK)
    {
        MDV_LOGE("Mutex creation failed with error %d", err);
        mdv_rollback(rollbacker);
        return err;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &datasync->mutex);

    datasync->start = mdv_eventfd(false);

    if (datasync->start == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("Eventfd creation failed");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_eventfd_close, datasync->start);

    atomic_init(&datasync->active, false);

    mdv_thread_attrs const attrs =
    {
        .stack_size = MDV_THREAD_STACK_SIZE
    };

    err = mdv_thread_create(&datasync->thread, &attrs, mdv_datasync_thread, datasync);

    if (err != MDV_OK)
    {
        MDV_LOGE("Thread creation failed with error %d", err);
        mdv_rollback(rollbacker);
        return err;
    }

    while(!mdv_datasync_is_active(datasync))
        mdv_sleep(100);

    mdv_rollbacker_push(rollbacker, mdv_datasync_stop, datasync);

    if (!mdv_vector_create(datasync->routes, 64, mdv_default_allocator))
    {
        MDV_LOGE("No memorty for routes");
        mdv_rollback(rollbacker);
        return MDV_NO_MEM;
    }

    mdv_rollbacker_free(rollbacker);

    return MDV_OK;
}


void mdv_datasync_free(mdv_datasync *datasync)
{
    mdv_datasync_stop(datasync);
    mdv_vector_free(datasync->routes);
    mdv_mutex_free(&datasync->mutex);
    mdv_eventfd_close(datasync->start);
    memset(datasync, 0, sizeof(*datasync));
}


mdv_errno mdv_datasync_update_routes(mdv_datasync *datasync)
{
    if (!mdv_datasync_is_active(datasync))
        return MDV_FAILED;

    mdv_routes routes;

    if (!mdv_vector_create(routes, 64, mdv_default_allocator))
    {
        MDV_LOGE("No memorty for routes");
        return MDV_NO_MEM;
    }

    mdv_errno err = mdv_routes_find(&routes, datasync->tracker);

    if (err != MDV_OK)
    {
        mdv_vector_free(routes);
        return err;
    }

    if (mdv_mutex_lock(&datasync->mutex) == MDV_OK)
    {
        mdv_vector_free(datasync->routes);
        datasync->routes = routes;

        MDV_LOGD("New routes count=%zu", mdv_vector_size(routes));

        for(size_t i = 0; i < mdv_vector_size(routes); ++i)
        {
            MDV_LOGD("Route[%zu]: %u", i, routes.data[i]);
        }

        mdv_mutex_unlock(&datasync->mutex);
        return MDV_OK;
    }

    mdv_vector_free(routes);

    return MDV_FAILED;
}


void mdv_datasync_start(mdv_datasync *datasync)
{
    uint64_t const signal = 1;
    mdv_write_all(datasync->start, &signal, sizeof signal);
}
