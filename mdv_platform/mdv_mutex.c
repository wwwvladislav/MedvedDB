#include "mdv_mutex.h"
#include "mdv_alloc.h"
#include "mdv_log.h"
#include <pthread.h>


/// @cond Doxygen_Suppress

struct mdv_mutex
{
    pthread_mutex_t mutex;
};

/// @endcond


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

