#include "mdv_threads.h"
#include "mdv_log.h"
#include "mdv_errno.h"
#include "mdv_alloc.h"
#include <time.h>
#include <pthread.h>
//#include <semaphore.h>


/// @cond Doxygen_Suppress

struct mdv_mutex
{
    pthread_mutex_t mutex;
};

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


/// @cond Doxygen_Suppress

typedef struct
{
    volatile int    is_started;
    mdv_thread_fn   fn;
    void           *arg;
} mdv_thread_arg;

/// @endcond


static void *mdv_thread_function(void *arg)
{
    mdv_thread_arg *thread_arg = (mdv_thread_arg*)arg;

    mdv_thread_fn fn = thread_arg->fn;

    arg = thread_arg->arg;

    thread_arg->is_started = 0xABCDEF;

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

    for(int i = 0; !thread_arg.is_started && i < 500; ++i)
        mdv_sleep(10);

    if (!thread_arg.is_started)
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


mdv_mutex * mdv_mutex_create()
{
    mdv_mutex *m = (mdv_mutex *)mdv_alloc(sizeof(mdv_mutex));

    if (m)
    {
        int err = pthread_mutex_init(&m->mutex, 0);

        if (err)
        {
            MDV_LOGE("mutex initialization failed with error %d", err);
            mdv_free(m);
            m = 0;
        }
    }
    else
        MDV_LOGE("mutex_create failed");

    return m;
}


void mdv_mutex_free(mdv_mutex *m)
{
    if (m)
    {
        pthread_mutex_destroy(&m->mutex);
        mdv_free(m);
    }
}


mdv_errno mdv_mutex_lock(mdv_mutex *m)
{
    int err = pthread_mutex_lock(&m->mutex);

    if(!err)
    {
        MDV_LOGD("Mutex %p locked", m);
        return MDV_OK;
    }

    return mdv_error();
}


mdv_errno mdv_mutex_trylock(mdv_mutex *m)
{
    int err = pthread_mutex_trylock(&m->mutex);

    if(!err)
    {
        MDV_LOGD("Mutex %p locked", m);
        return MDV_OK;
    }

    return mdv_error();
}


mdv_errno mdv_mutex_unlock(mdv_mutex *m)
{
    int err = pthread_mutex_unlock(&m->mutex);

    if(!err)
    {
        MDV_LOGD("Mutex %p unlocked", m);
        return MDV_OK;
    }

    return mdv_error();
}
