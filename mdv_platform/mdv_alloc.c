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


struct mdv_mempool_base
{
    size_t  capacity;
    size_t  size;
    uint8_t buff[1];
};


void *mdv_mempool_ptr(void *mpool)
{
    struct mdv_mempool_base *mempool = (struct mdv_mempool_base *)mpool;
    return mempool->buff;
}


size_t mdv_mempool_size(void *mpool)
{
    struct mdv_mempool_base *mempool = (struct mdv_mempool_base *)mpool;
    return mempool->size;
}


size_t mdv_mempool_capacity(void *mpool)
{
    struct mdv_mempool_base *mempool = (struct mdv_mempool_base *)mpool;
    return mempool->capacity;
}


void *mdv_palloc(void *mpool, size_t size)
{
    struct mdv_mempool_base *mempool = (struct mdv_mempool_base *)mpool;

    if (mempool->size + size > mempool->capacity)
    {
        MDV_LOGE("palloc(%zu) failed. Available space in mempool is (%zu)", size, mempool->capacity - mempool->size);
        return 0;
    }

    uint8_t *ptr = mempool->buff + mempool->size;
    mempool->size += size;

    return ptr;
}
