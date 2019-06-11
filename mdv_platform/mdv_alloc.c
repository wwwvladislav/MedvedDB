#include "mdv_alloc.h"
#include "mdv_log.h"
#include <stdlib.h>


void *mdv_alloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr)
        MDV_LOGE("malloc(%zu) failed", size);
    MDV_LOGD("malloc: %p:%zu", ptr, size);
    return ptr;
}


void *mdv_aligned_alloc(size_t alignment, size_t size)
{
    void *ptr = aligned_alloc(alignment, size);
    if (!ptr)
        MDV_LOGE("aligned_alloc(%zu) failed", size);
    MDV_LOGD("aligned_alloc: %p:%zu", ptr, size);
    return ptr;
}


void mdv_free(void *ptr)
{
    MDV_LOGD("free: %p", ptr);
    free(ptr);
}
