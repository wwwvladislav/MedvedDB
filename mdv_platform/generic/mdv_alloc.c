#include "mdv_alloc.h"
#include <mdv_log.h>
#include <stdatomic.h>
#include <string.h>
#include <stdlib.h>


void * mdv_alloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr)
        MDV_LOGE("malloc(%zu) failed", size);
    return ptr;
}


void * mdv_realloc(void *ptr, size_t size)
{
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr)
        MDV_LOGE("realloc(%p, %zu) failed", ptr, size);
    return new_ptr;
}


void mdv_free(void *ptr)
{
    if (ptr)
        free(ptr);
}


mdv_allocator const mdv_default_allocator =
{
    .alloc      = &mdv_alloc,
    .realloc    = &mdv_realloc,
    .free       = &mdv_free
};
