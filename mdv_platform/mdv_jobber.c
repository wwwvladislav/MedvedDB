#include "mdv_jobber.h"
#include "mdv_rollbacker.h"
#include "mdv_queuefd.h"
#include "mdv_alloc.h"
#include "mdv_log.h"



enum
{
    MDV_JOBBER_QUEUE_SIZE = 256             ///< Job queues size
};


/// Job function type with 1 argument
typedef void(*mdv_job_fn_1)(void *);

/// Job function type with 2 arguments
typedef void(*mdv_job_fn_2)(void *, void *);

/// Job function type with 3 arguments
typedef void(*mdv_job_fn_3)(void *, void *, void *);


typedef mdv_queuefd(mdv_job*, MDV_JOBBER_QUEUE_SIZE) mdv_jobber_queue;


struct mdv_jobber
{
    mdv_threadpool     *threads;
    size_t              queue_count;
    mdv_jobber_queue    jobs[1];
};


mdv_jobber * mdv_jobber_create(mdv_jobber_config const *config)
{
    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_jobber *jobber = mdv_alloc(offsetof(mdv_jobber, jobs) + config->queue.count * sizeof(mdv_jobber_queue), "jobber");

    if (!jobber)
    {
        MDV_LOGE("No memory for job scheduler");
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, jobber, "jobber");


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
            for(size_t j = 0; j < i; ++j)
                mdv_queuefd_free(jobber->jobs[j]);

            MDV_LOGE("Jobs queue creation failed");
            mdv_rollback(rollbacker);
            return 0;
        }
    }

    return jobber;
}


void mdv_jobber_free(mdv_jobber *jobber)
{
    if (jobber)
    {
        mdv_threadpool_free(jobber->threads);

        for(size_t i = 0; i < jobber->queue_count; ++i)
            mdv_queuefd_free(jobber->jobs[i]);

        mdv_free(jobber, "jobber");
    }
}
