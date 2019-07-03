#include "mdv_threads.h"
#include "mdv_log.h"
#include "mdv_alloc.h"
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>


/// @cond Doxygen_Suppress

typedef struct
{
    atomic_bool     is_started;
    mdv_thread_fn   fn;
    void           *arg;
} mdv_thread_arg;

/// @endcond


void mdv_sleep(size_t msec)
{
    struct timespec t =
    {
        .tv_sec = msec / 1000,
        .tv_nsec = (msec % 1000) * 1000000
    };

    nanosleep(&t, 0);
}


static void *mdv_thread_function(void *arg)
{
    mdv_thread_arg *thread_arg = (mdv_thread_arg*)arg;

    mdv_thread_fn fn = thread_arg->fn;

    arg = thread_arg->arg;

    atomic_store_explicit(&thread_arg->is_started, 1, memory_order_release);

    mdv_alloc_thread_initialize();

    void *ret = fn(arg);

    mdv_alloc_thread_finalize();

    return ret;
}


mdv_errno mdv_thread_create(mdv_thread *thread, mdv_thread_fn fn, void *arg)
{
    pthread_t pthread;

    *thread = 0;

    mdv_thread_arg thread_arg =
    {
        .is_started = 0,
        .fn = fn,
        .arg = arg
    };

    int err = pthread_create(&pthread, 0, mdv_thread_function, &thread_arg);

    if (err)
    {
        err = mdv_error();
        MDV_LOGE("Thread starting was failed with error %d", err);
        return err;
    }

    for(int i = 0;
        !atomic_load_explicit(&thread_arg.is_started, memory_order_acquire) && i < 500;
        ++i)
        mdv_sleep(10);

    if (!atomic_load_explicit(&thread_arg.is_started, memory_order_acquire))
        MDV_LOGW("Thread is starting too long: %p", thread);

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

