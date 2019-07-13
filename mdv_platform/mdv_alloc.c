#include "mdv_alloc.h"
#include "mdv_log.h"
#include <rpmalloc.h>
#include <stdatomic.h>


#define MDV_COUNT_ALLOCS_STAT


#ifdef MDV_COUNT_ALLOCS_STAT

    static atomic_int MDV_ALLOCS = 0;

    #define MDV_ALLOCATION(fn, ptr, size)                                                           \
        {                                                                                           \
            int allocs_counter = atomic_fetch_add_explicit(&MDV_ALLOCS, 1, memory_order_relaxed);   \
            MDV_LOGD("%s: %p:%zu, (allocations: %d)", #fn, ptr, size, allocs_counter + 1);          \
        }

    #define MDV_DEALLOCATION(fn, ptr)                                                               \
        {                                                                                           \
            int allocs_counter = atomic_fetch_sub_explicit(&MDV_ALLOCS, 1, memory_order_relaxed);   \
            MDV_LOGD("%s: %p, (allocations: %d)", #fn, ptr, allocs_counter - 1);                    \
        }

#else

    #define MDV_ALLOCATION(fn, ptr, size)           \
        MDV_LOGD("%s: %p:%zu", #fn, ptr, size);

    #define MDV_DEALLOCATION(fn, ptr)               \
            MDV_LOGD("%s: %p", #fn, ptr);

#endif


enum
{
    MDV_THREAD_LOCAL_STORAGE_SIZE = 128 * 1024
};


static _Thread_local struct
{
    char buffer[MDV_THREAD_LOCAL_STORAGE_SIZE];
    int  is_used;
}
_thread_local_tmp_buff =
{
    .is_used = 0
};


int mdv_alloc_initialize()
{
    return rpmalloc_initialize();
}


void mdv_alloc_finalize()
{
    rpmalloc_finalize();
}


void mdv_alloc_thread_initialize()
{
    rpmalloc_thread_initialize();
}


void mdv_alloc_thread_finalize()
{
    rpmalloc_thread_finalize();
}


void * mdv_alloc(size_t size)
{
    void *ptr = rpmalloc(size);
    if (!ptr)
        MDV_LOGE("malloc(%zu) failed", size);
    else
        MDV_ALLOCATION(malloc, ptr, size);
    return ptr;
}


void * mdv_aligned_alloc(size_t alignment, size_t size)
{
    void *ptr = rpaligned_alloc(alignment, size);
    if (!ptr)
        MDV_LOGE("aligned_alloc(%zu) failed", size);
    else
        MDV_ALLOCATION(aligned_alloc, ptr, size);
    return ptr;
}


void * mdv_realloc(void *ptr, size_t size)
{
    void *new_ptr = rprealloc(ptr, size);
    if (!new_ptr)
        MDV_LOGE("realloc(%p, %zu) failed", ptr, size);
    else
        MDV_ALLOCATION(realloc, new_ptr, size);
    return new_ptr;
}


void *mdv_alloc_tmp(size_t size)
{
    if (size < sizeof _thread_local_tmp_buff.buffer)
    {
        if (!_thread_local_tmp_buff.is_used)
        {
            _thread_local_tmp_buff.is_used = 1;
            return _thread_local_tmp_buff.buffer;
        }
        MDV_LOGW("Thread local buffer is busy. Dynamic allocation is performed.");
    }
    return mdv_alloc(size);
}


void mdv_free(void *ptr)
{
    if (ptr)
    {
        if (ptr != _thread_local_tmp_buff.buffer)
        {
            rpfree(ptr);
            MDV_DEALLOCATION(free, ptr);
        }
        else
            _thread_local_tmp_buff.is_used = 0;
    }
}
