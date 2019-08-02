#pragma once
#include "../minunit.h"
#include <mdv_jobber.h>
#include <mdv_condvar.h>


static void mdv_test_job(mdv_job_base *job, void *arg1, void *arg2)
{
    (void)job;

    int *res = (int *)arg1;

    mdv_condvar *cv = (mdv_condvar *)arg2;

    if (++*res >= 42)
        mdv_condvar_signal(cv);
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

    mdv_condvar cv;
    mu_check(mdv_condvar_create(&cv) == MDV_OK);

    int res = 0;

    mdv_job(2) job =
    {
        .fn         = (mdv_job_fn)&mdv_test_job,
        .args_count = 2,
        .arg =
        {
            &res,
            &cv
        }
    };

    for(int i = 0; i < 42; ++i)
        mu_check(mdv_jobber_push(jobber, (mdv_job_base*)&job) == MDV_OK);

    mu_check(mdv_condvar_timedwait(&cv, 100) == MDV_OK);

    mu_check(res == 42);

    mdv_condvar_free(&cv);

    mdv_jobber_free(jobber);
}
