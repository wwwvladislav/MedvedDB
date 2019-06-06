#pragma once
#include <stddef.h>
#include <stdint.h>

void *mdv_alloc(size_t size);
void *mdv_aligned_alloc(size_t alignment, size_t size);
void  mdv_free(void *ptr);


#define mdv_mempool(name, sz)   \
    struct                      \
    {                           \
        size_t  capacity;       \
        size_t  size;           \
        uint8_t buff[sz];       \
    } name =                    \
    {                           \
        .capacity = sz,         \
        .size = 0               \
    };

#define mdv_pfree(mpool)        \
    (mpool).size = 0;

void *mdv_palloc(void *mpool, size_t size);
