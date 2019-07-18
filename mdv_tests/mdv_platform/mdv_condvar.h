#pragma once
#include "../minunit.h"
#include <mdv_condvar.h>
#include <mdv_threads.h>
#include <stdint.h>


static void * mdv_cv_thread(void *data)
{
    mdv_condvar *cv = (mdv_condvar *)data;
    mdv_condvar_wait(cv);
    return 0;
}


MU_TEST(platform_condvar)
{
    mdv_condvar cv;

    mu_check(mdv_condvar_create(&cv) == MDV_OK);

    mu_check(mdv_condvar_timedwait(&cv, 5) == MDV_ETIMEDOUT);

    mdv_thread_attrs thread_attrs =
    {
        .stack_size = MDV_THREAD_STACK_SIZE
    };

    mdv_thread thread;
    mu_check(mdv_thread_create(&thread, &thread_attrs, &mdv_cv_thread, &cv) == MDV_OK);

    mdv_sleep(100);

    mu_check(mdv_condvar_signal(&cv) == MDV_OK);

    mu_check(mdv_thread_join(thread) == MDV_OK);

    mdv_condvar_free(&cv);
}

