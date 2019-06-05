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


void mdv_free(void *ptr)
{
    MDV_LOGD("free: %p", ptr);
    free(ptr);
}

