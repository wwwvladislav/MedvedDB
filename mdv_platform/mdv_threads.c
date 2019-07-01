#include "mdv_threads.h"
#include "mdv_log.h"
#include "mdv_errno.h"
#include <time.h>
#include <pthread.h>
//#include <semaphore.h>


void mdv_sleep(size_t msec)
{
    struct timespec t =
    {
        .tv_sec = msec / 1000,
        .tv_nsec = (msec % 1000) * 1000000
    };

    nanosleep(&t, 0);
}


mdv_errno mdv_thread_create(mdv_thread *thread, mdv_thread_fn fn, void *arg)
{
    pthread_t pthread;

    *thread = 0;

    int err = pthread_create(&pthread, 0, fn, arg);

    if (err)
    {
        err = mdv_error();
        MDV_LOGE("Thread starting was failed with error %d", err);
        return err;
    }

    *(pthread_t*)thread = pthread;

    MDV_LOGI("New thread started: %p", thread);

    return MDV_OK;
}


mdv_thread mdv_thread_self()
{
    mdv_thread thread = 0;
    *(pthread_t*)&thread = pthread_self();
    return thread;
}


int mdv_thread_equal(mdv_thread t1, mdv_thread t2)
{
    return pthread_equal(*(pthread_t*)&t1, *(pthread_t*)&t2);
}


mdv_errno mdv_thread_join(mdv_thread thread)
{
    MDV_LOGI("Join thread: %p", thread);

    int err = pthread_join(*(pthread_t*)&thread, 0);

    if (err)
    {
        err = mdv_error();
        MDV_LOGE("Thread joining was failed with error %d", err);
        return err;
    }

    return MDV_OK;
}


