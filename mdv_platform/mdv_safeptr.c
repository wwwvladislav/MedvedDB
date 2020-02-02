#include "mdv_safeptr.h"
#include "mdv_alloc.h"
#include "mdv_mutex.h"
#include "mdv_log.h"


struct mdv_safeptr
{
    void                   *ptr;
    mdv_mutex               mutex;
    mdv_safeptr_retain_fn   retain;
    mdv_safeptr_release_fn  release;
};


mdv_safeptr * mdv_safeptr_create(void *ptr,
                                 mdv_safeptr_retain_fn retain,
                                 mdv_safeptr_release_fn release)
{
    mdv_safeptr *safeptr = mdv_alloc(sizeof(mdv_safeptr), "safeptr");

    if (!safeptr)
    {
        MDV_LOGE("No memory for safeptr");
        return 0;
    }

    if (mdv_mutex_create(&safeptr->mutex) != MDV_OK)
    {
        mdv_free(safeptr, "safeptr");
        return 0;
    }

    safeptr->ptr = ptr ? retain(ptr) : 0;
    safeptr->retain = retain;
    safeptr->release = release;

    return safeptr;
}


void mdv_safeptr_free(mdv_safeptr *safeptr)
{
    if (safeptr)
    {
        if(safeptr->ptr)
            safeptr->release(safeptr->ptr);
        mdv_mutex_free(&safeptr->mutex);
        mdv_free(safeptr, "safeptr");
    }
}


mdv_errno mdv_safeptr_set(mdv_safeptr *safeptr, void *ptr)
{
    mdv_errno err = mdv_mutex_lock(&safeptr->mutex);

    if (err == MDV_OK)
    {
        if(safeptr->ptr)
            safeptr->release(safeptr->ptr);
        safeptr->ptr = ptr ? safeptr->retain(ptr) : 0;
        mdv_mutex_unlock(&safeptr->mutex);
    }

    return err;
}


void * mdv_safeptr_get(mdv_safeptr *safeptr)
{
    void *ptr = 0;

    if (mdv_mutex_lock(&safeptr->mutex) == MDV_OK)
    {
        ptr = safeptr->ptr
                ? safeptr->retain(safeptr->ptr)
                : 0;
        mdv_mutex_unlock(&safeptr->mutex);
    }

    return ptr;
}
