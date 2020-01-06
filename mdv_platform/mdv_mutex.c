#include "mdv_mutex.h"
#include "mdv_log.h"


mdv_errno mdv_mutex_create(mdv_mutex *mutex)
{
    int err = pthread_mutex_init(mutex, 0);

    if (err)
    {
        MDV_LOGE("mutex initialization failed with error %d", err);
        return MDV_FAILED;
    }

    return MDV_OK;
}


void mdv_mutex_free(mdv_mutex *mutex)
{
    pthread_mutex_destroy(mutex);
}


mdv_errno mdv_mutex_lock(mdv_mutex *mutex)
{
    int err = pthread_mutex_lock(mutex);

    if(!err)
    {
        MDV_LOGD("Mutex %p locked", mutex);
        return MDV_OK;
    }
    else
        MDV_LOGD("Mutex %p lock failed", mutex);

    return mdv_error();
}


mdv_errno mdv_mutex_trylock(mdv_mutex *mutex)
{
    int err = pthread_mutex_trylock(mutex);

    if(!err)
    {
        MDV_LOGD("Mutex %p locked", mutex);
        return MDV_OK;
    }

    return mdv_error();
}


mdv_errno mdv_mutex_unlock(mdv_mutex *mutex)
{
    int err = pthread_mutex_unlock(mutex);

    if(!err)
    {
        MDV_LOGD("Mutex %p unlocked", mutex);
        return MDV_OK;
    }

    return mdv_error();
}
