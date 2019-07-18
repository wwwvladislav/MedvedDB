#include "mdv_condvar.h"
#include "mdv_alloc.h"
#include "mdv_log.h"
#include <time.h>
#include <errno.h>


mdv_errno mdv_condvar_create(mdv_condvar *cv)
{
    if (pthread_mutex_init(&cv->mutex, 0) != 0)
    {
        MDV_LOGE("condvar mutex initialization failed with error %d", mdv_error());
        return MDV_FAILED;
    }

    pthread_condattr_t condattr;

    if (pthread_condattr_init(&condattr) != 0)
    {
        MDV_LOGE("condvar attributes initialization failed with error %d", mdv_error());
        pthread_condattr_destroy(&condattr);
        pthread_mutex_destroy(&cv->mutex);
        return MDV_FAILED;
    }

    if (pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC) != 0)
    {
        MDV_LOGE("condvar attributes initialization failed with error %d", mdv_error());
        pthread_condattr_destroy(&condattr);
        pthread_mutex_destroy(&cv->mutex);
        return MDV_FAILED;
    }


    if (pthread_cond_init(&cv->cv, &condattr) != 0)
    {
        MDV_LOGE("condvar initialization failed with error %d", mdv_error());
        pthread_condattr_destroy(&condattr);
        pthread_mutex_destroy(&cv->mutex);
        return MDV_FAILED;
    }

    pthread_condattr_destroy(&condattr);

    return MDV_OK;
}


void mdv_condvar_free(mdv_condvar *cv)
{
    if (cv)
    {
        pthread_mutex_destroy(&cv->mutex);
        pthread_cond_destroy(&cv->cv);
    }
}


mdv_errno mdv_condvar_signal(mdv_condvar *cv)
{
    mdv_errno err = MDV_FAILED;

    if (pthread_mutex_lock(&cv->mutex) == 0)
    {
        int cv_err = pthread_cond_signal(&cv->cv);

        if (cv_err == 0)
            err = MDV_OK;
        else
        {
            err = cv_err;
            MDV_LOGE("condvar signaling failed with error %d", err);
        }

        pthread_mutex_unlock(&cv->mutex);
    }
    else
        MDV_LOGE("condvar mutex locking failed");

    return err;
}


mdv_errno mdv_condvar_wait(mdv_condvar *cv)
{
    mdv_errno err = MDV_FAILED;

    if (pthread_mutex_lock(&cv->mutex) == 0)
    {
        int cv_err = pthread_cond_wait(&cv->cv, &cv->mutex);

        if (cv_err == 0)
            err = MDV_OK;
        else
        {
            err = cv_err;
            MDV_LOGE("condvar waiting failed with error %d", err);
        }

        pthread_mutex_unlock(&cv->mutex);
    }
    else
        MDV_LOGE("condvar mutex locking failed");

    return err;
}


mdv_errno mdv_condvar_timedwait(mdv_condvar *cv, size_t duration)
{
    mdv_errno err = MDV_FAILED;

    struct timespec t;

    if (clock_gettime(CLOCK_MONOTONIC, &t) != 0)
    {
        err = mdv_error();
        MDV_LOGE("clock_gettime failed with error %d", err);
        return err;
    }

    t.tv_sec += duration / 1000;
    t.tv_nsec += (duration % 1000) * 1000000;

    if (pthread_mutex_lock(&cv->mutex) == 0)
    {
        int cv_err = pthread_cond_timedwait(&cv->cv, &cv->mutex, &t);

        if (cv_err == 0)
            err = MDV_OK;
        else if (cv_err == ETIMEDOUT)
            err = MDV_ETIMEDOUT;
        else
        {
            err = cv_err;
            MDV_LOGE("condvar waiting failed with error %d", err);
        }

        pthread_mutex_unlock(&cv->mutex);
    }
    else
        MDV_LOGE("condvar mutex locking failed");

    return err;
}

