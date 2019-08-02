#pragma once
#include "../minunit.h"
#include <mdv_jobber.h>
#include <mdv_condvar.h>


typedef struct mdv_test_job_data
{
    int         counter;
    mdv_condvar cv;
} mdv_test_job_data;


static void mdv_test_job(mdv_job_base *job)
{
    mdv_test_job_data *data = (mdv_test_job_data *)job->data;

    if (++data->counter >= 42)
        mdv_condvar_signal(&data->cv);
}


MU_TEST(platform_jobber)
{
    mdv_jobber_config const config =
    {
        .threadpool =
        {
            .size = 2,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .queue =
        {
            .count = 4
        }
    };

    mdv_jobber *jobber = mdv_jobber_create(&config);
    mu_check(jobber);


    mdv_job(mdv_test_job_data) job =
    {
        .fn         = (mdv_job_fn)&mdv_test_job,
        .finalize   = 0,
        .data =
        {
            .counter = 0
        }
    };

    mu_check(mdv_condvar_create(&job.data.cv) == MDV_OK);

    for(int i = 0; i < 42; ++i)
        mu_check(mdv_jobber_push(jobber, (mdv_job_base*)&job) == MDV_OK);

    mu_check(mdv_condvar_timedwait(&job.data.cv, 100) == MDV_OK);

    mu_check(job.data.counter == 42);

    mdv_condvar_free(&job.data.cv);

    mdv_jobber_free(jobber);
}
