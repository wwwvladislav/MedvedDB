#include "mdv_committer.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <mdv_epoll.h>


typedef struct mdv_committer_context
{
    uint32_t        peer_id;        ///< Source peer identifier
    mdv_committer  *committer;      ///< Data committer
    mdv_uuid        storage;        ///< Data storage UUID for synchronization
} mdv_committer_context;


typedef mdv_job(mdv_committer_context)     mdv_committer_job;


static bool mdv_committer_is_active(mdv_committer *committer)
{
    return atomic_load(&committer->active);
}


static void mdv_committer_fn(mdv_job_base *job)
{
    mdv_committer_context *ctx      = (mdv_committer_context *)job->data;
    mdv_committer         *committer = ctx->committer;

    if (!mdv_committer_is_active(committer))
        return;

    if (!mdv_tablespace_log_apply(committer->tablespace, &ctx->storage, ctx->peer_id))
        MDV_LOGE("Transaction log was not applied");
}


static void mdv_committer_finalize(mdv_job_base *job)
{
    mdv_committer_context *ctx = (mdv_committer_context *)job->data;
    mdv_committer         *committer = ctx->committer;
    atomic_fetch_sub_explicit(&committer->active_jobs, 1, memory_order_relaxed);
    mdv_free(job, "committer_job");
}


static void mdv_committer_job_emit(mdv_committer *committer, uint32_t peer_id, mdv_uuid const *storage)
{
    mdv_committer_job *job = mdv_alloc(sizeof(mdv_committer_job), "committer_job");

    if (!job)
    {
        MDV_LOGE("No memory for data committer job");
        return;
    }

    job->fn             = mdv_committer_fn;
    job->finalize       = mdv_committer_finalize;
    job->data.peer_id   = peer_id;
    job->data.committer = committer;
    job->data.storage   = *storage;

    mdv_errno err = mdv_jobber_push(committer->jobber, (mdv_job_base*)job);

    if (err != MDV_OK)
    {
        MDV_LOGE("Data committer job failed");
        mdv_free(job, "committer_job");
    }
    else
    {
        atomic_fetch_add_explicit(&committer->active_jobs, 1, memory_order_relaxed);
    }
}


// Main data commit thread generates asynchronous jobs for each peer.
static void mdv_committer_main(mdv_committer *committer)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);


    mdv_vector *src_peers = mdv_tracker_nodes(committer->tracker);

    if (!src_peers)
    {
        mdv_rollback(rollbacker);
        return;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, src_peers);

    if (mdv_vector_empty(src_peers))
    {
        mdv_rollback(rollbacker);
        return;
    }

/* TODO
    mdv_vector *storages = mdv_tablespace_storages(committer->tablespace);

    if (!storages)
    {
        mdv_rollback(rollbacker);
        return;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, storages);

    if (mdv_vector_empty(storages))
    {
        mdv_rollback(rollbacker);
        return;
    }


    mdv_vector_foreach(storages, mdv_uuid, strg)
    {
        mdv_cfstorage *storage = mdv_tablespace_cfstorage(committer->tablespace, strg);

        if (!storage)
            continue;

        mdv_vector_foreach(src_peers, uint32_t, src)
        {
            if (mdv_cfstorage_log_changed(storage, *src))
                mdv_committer_job_emit(committer, *src, strg);
        }
    }
*/
    mdv_rollback(rollbacker);
}


static void * mdv_committer_thread(void *arg)
{
    mdv_committer *committer = arg;

    atomic_store(&committer->active, true);

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

    if (mdv_epoll_add(epfd, committer->start, event) != MDV_OK)
    {
        MDV_LOGE("epoll event registration failed");
        mdv_epoll_close(epfd);
        return 0;
    }

    while(mdv_committer_is_active(committer))
    {
        mdv_epoll_event events[1];
        uint32_t size = sizeof events / sizeof *events;

        mdv_epoll_wait(epfd, events, &size, -1);

        if (!mdv_committer_is_active(committer))
            break;

        if (size)
        {
            if (events[0].events & MDV_EPOLLIN)
            {
                uint64_t signal;
                size_t len = sizeof(signal);

                while(mdv_read(committer->start, &signal, &len) == MDV_EAGAIN);

                mdv_committer_main(committer);
            }
        }
    }

    mdv_epoll_close(epfd);

    return 0;
}


mdv_errno mdv_committer_create(mdv_committer *committer,
                               mdv_tablespace *tablespace,
                               mdv_tracker *tracker,
                               mdv_jobber *jobber)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    committer->tablespace = tablespace;
    committer->tracker = mdv_tracker_retain(tracker);
    committer->jobber = mdv_jobber_retain(jobber);

    mdv_rollbacker_push(rollbacker, mdv_tracker_release, committer->tracker);
    mdv_rollbacker_push(rollbacker, mdv_jobber_release, committer->jobber);

    atomic_init(&committer->active_jobs, 0);

    committer->start = mdv_eventfd(false);

    if (committer->start == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("Eventfd creation failed");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_eventfd_close, committer->start);

    atomic_init(&committer->active, false);

    mdv_thread_attrs const attrs =
    {
        .stack_size = MDV_THREAD_STACK_SIZE
    };

    mdv_errno err = mdv_thread_create(&committer->thread, &attrs, mdv_committer_thread, committer);

    if (err != MDV_OK)
    {
        MDV_LOGE("Thread creation failed with error %d", err);
        mdv_rollback(rollbacker);
        return err;
    }

    while(!mdv_committer_is_active(committer))
        mdv_sleep(100);

    mdv_rollbacker_push(rollbacker, mdv_committer_stop, committer);

    mdv_rollbacker_free(rollbacker);

    return MDV_OK;
}


void mdv_committer_stop(mdv_committer *committer)
{
    if (mdv_committer_is_active(committer))
    {
        atomic_store(&committer->active, false);

        uint64_t const signal = 1;
        mdv_write_all(committer->start, &signal, sizeof signal);

        mdv_thread_join(committer->thread);

        while(atomic_load_explicit(&committer->active_jobs, memory_order_relaxed) > 0)
            mdv_sleep(100);
    }
}


void mdv_committer_free(mdv_committer *committer)
{
    mdv_committer_stop(committer);
    mdv_jobber_release(committer->jobber);
    mdv_tracker_release(committer->tracker);
    mdv_eventfd_close(committer->start);
    memset(committer, 0, sizeof(*committer));
}


void mdv_committer_start(mdv_committer *committer)
{
    uint64_t const signal = 1;
    mdv_write_all(committer->start, &signal, sizeof signal);
}

