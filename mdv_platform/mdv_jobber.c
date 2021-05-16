#include "mdv_jobber.h"
#include "mdv_rollbacker.h"
#include "mdv_queuefd.h"
#include "mdv_alloc.h"
#include "mdv_log.h"
#include <stdatomic.h>


/// @cond Doxygen_Suppress

enum
{
    MDV_JOBBER_QUEUE_SIZE           = 256,  ///< Job queues size
    MDV_JOBBER_QUEUE_PUSH_ATTEMPTS  = 64    ///< Number of job push attempts
};


typedef mdv_queuefd(mdv_job_base*, MDV_JOBBER_QUEUE_SIZE) mdv_jobber_queue;


struct mdv_jobber
{
    atomic_uint_fast32_t    rc;
    mdv_threadpool         *threads;
    size_t                  queue_count;
    atomic_size_t           idx;
    mdv_jobber_queue        jobs[1];
};


typedef struct mdv_jobber_context
{
    mdv_jobber_queue *jobs;
} mdv_jobber_context;


typedef mdv_threadpool_task(mdv_jobber_context) mdv_jobber_task;


/// @endcond


static void mdv_job_handler(uint32_t events, mdv_threadpool_task_base *task_base)
{
    mdv_jobber_task *jobber_task = (mdv_jobber_task*)task_base;
    mdv_jobber_context *jobber_context = &jobber_task->context;

    if (events & MDV_EPOLLIN)
    {
        mdv_job_base *job = 0;

        if (mdv_queuefd_pop(*jobber_context->jobs, job))
        {
            job->fn(job);

            if (job->finalize)
                job->finalize(job);
        }
    }
}


mdv_jobber * mdv_jobber_create(mdv_jobber_config const *config)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    mdv_jobber *jobber = mdv_alloc(offsetof(mdv_jobber, jobs) + config->queue.count * sizeof(mdv_jobber_queue));

    if (!jobber)
    {
        MDV_LOGE("No memory for job scheduler");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, jobber);

    atomic_init(&jobber->rc, 1);
    atomic_init(&jobber->idx, 0);


    jobber->threads = mdv_threadpool_create(&config->threadpool);

    if (!jobber->threads)
    {
        MDV_LOGE("Threadpool creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_threadpool_free, jobber->threads);


    jobber->queue_count = config->queue.count ? config->queue.count : 1;

    for(size_t i = 0; i < jobber->queue_count; ++i)
    {
        mdv_queuefd_init(jobber->jobs[i]);

        if (!mdv_queuefd_ok(jobber->jobs[i]))
        {
            mdv_threadpool_stop(jobber->threads);

            for(size_t j = 0; j < i; ++j)
                mdv_queuefd_free(jobber->jobs[j]);

            MDV_LOGE("Jobs queue creation failed");
            mdv_rollback(rollbacker);
            return 0;
        }

        mdv_jobber_task task =
        {
            .fd = jobber->jobs[i].event,
            .fn = mdv_job_handler,
            .context_size = sizeof(mdv_jobber_context),
            .context =
            {
                .jobs = jobber->jobs + i
            }
        };

        if (!mdv_threadpool_add(jobber->threads, MDV_EPOLLEXCLUSIVE | MDV_EPOLLIN | MDV_EPOLLERR, (mdv_threadpool_task_base const *)&task))
        {
            mdv_threadpool_stop(jobber->threads);

            for(size_t j = 0; j < i; ++j)
                mdv_queuefd_free(jobber->jobs[j]);

            MDV_LOGE("Jobs queue registration failed");
            mdv_rollback(rollbacker);
            return 0;
        }
    }

    mdv_rollbacker_free(rollbacker);

    return jobber;
}


static void mdv_jobber_free(mdv_jobber *jobber)
{
    mdv_threadpool_stop(jobber->threads);
    mdv_threadpool_free(jobber->threads);

    for(size_t i = 0; i < jobber->queue_count; ++i)
    {
        if (mdv_queuefd_size(jobber->jobs[i]))
        {
            mdv_job_base *job = 0;

            while(mdv_queuefd_pop(jobber->jobs[i], job))
            {
                if (job->finalize)
                    job->finalize(job);
            }
        }
        mdv_queuefd_free(jobber->jobs[i]);
    }

    mdv_free(jobber);
}


mdv_jobber * mdv_jobber_retain(mdv_jobber *jobber)
{
    atomic_fetch_add_explicit(&jobber->rc, 1, memory_order_acquire);
    return jobber;
}


void mdv_jobber_release(mdv_jobber *jobber)
{
    if (jobber
        && atomic_fetch_sub_explicit(&jobber->rc, 1, memory_order_release) == 1)
    {
        mdv_jobber_free(jobber);
    }
}


mdv_errno mdv_jobber_push(mdv_jobber *jobber, mdv_job_base *job)
{
    size_t idx = atomic_load_explicit(&jobber->idx, memory_order_relaxed);

    while (!atomic_compare_exchange_weak(&jobber->idx,
                                         &idx,
                                         (idx + 1) % jobber->queue_count));

    idx = (idx + 1) % jobber->queue_count;

    for(size_t i = 0; i < MDV_JOBBER_QUEUE_PUSH_ATTEMPTS; ++i)
    {
        if (mdv_queuefd_push(jobber->jobs[idx], job))
            return MDV_OK;
        mdv_thread_yield();
    }

    return MDV_FAILED;
}
