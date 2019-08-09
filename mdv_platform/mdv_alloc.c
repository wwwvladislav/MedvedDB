#include "mdv_alloc.h"
#include "mdv_stack.h"
#include "mdv_log.h"
#include <rpmalloc.h>
#include <stdatomic.h>


#define MDV_COUNT_ALLOCS_STAT


#ifdef MDV_COUNT_ALLOCS_STAT

    static atomic_int MDV_ALLOCS = 0;

    #define MDV_ALLOCATION(fn, ptr, size, name)                                                         \
        {                                                                                               \
            int allocs_counter = atomic_fetch_add_explicit(&MDV_ALLOCS, 1, memory_order_relaxed);       \
            MDV_LOGD("%s(%p:%zu) '%s' (allocations: %d)", #fn, ptr, size, name, allocs_counter + 1);    \
        }

    #define MDV_REALLOCATION(fn, ptr, size, name)                                                       \
        {                                                                                               \
            int allocs_counter = atomic_load_explicit(&MDV_ALLOCS, memory_order_relaxed);               \
            MDV_LOGD("%s(%p:%zu) '%s' (allocations: %d)", #fn, ptr, size, name, allocs_counter);        \
        }

    #define MDV_DEALLOCATION(fn, ptr, name)                                                             \
        {                                                                                               \
            int allocs_counter = atomic_fetch_sub_explicit(&MDV_ALLOCS, 1, memory_order_relaxed);       \
            MDV_LOGD("%s(%p) '%s' (allocations: %d)", #fn, ptr, name, allocs_counter - 1);              \
        }

#else

    #define MDV_ALLOCATION(fn, ptr, size, name)             \
        MDV_LOGD("%s(%p:%zu) '%s'", #fn, ptr, size, name);

    #define MDV_REALLOCATION(fn, ptr, size, name)           \
        MDV_LOGD("%s(%p:%zu) '%s'", #fn, ptr, size, name);

    #define MDV_DEALLOCATION(fn, ptr, name)                 \
            MDV_LOGD("%s(%p) '%s'", #fn, ptr, name);

#endif


enum
{
    MDV_THREAD_LOCAL_STORAGE_SIZE = 256 * 1024
};


static _Thread_local mdv_stack(uint8_t, MDV_THREAD_LOCAL_STORAGE_SIZE) _thread_local_buff =
{
    .capacity = MDV_THREAD_LOCAL_STORAGE_SIZE,
    .size = 0
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


void * mdv_alloc(size_t size, char const *name)
{
    void *ptr = rpmalloc(size);
    if (!ptr)
        MDV_LOGE("malloc(%zu) failed", size);
    else
        MDV_ALLOCATION(malloc, ptr, size, name);
    return ptr;
}


void * mdv_aligned_alloc(size_t alignment, size_t size, char const *name)
{
    void *ptr = rpaligned_alloc(alignment, size);
    if (!ptr)
        MDV_LOGE("aligned_alloc(%zu) failed", size);
    else
        MDV_ALLOCATION(aligned_alloc, ptr, size, name);
    return ptr;
}


void * mdv_realloc(void *ptr, size_t size, char const *name)
{
    void *new_ptr = rprealloc(ptr, size);
    if (!new_ptr)
        MDV_LOGE("realloc(%p, %zu) failed", ptr, size);
    else
        MDV_REALLOCATION(realloc, new_ptr, size, name);
    return new_ptr;
}


void *mdv_realloc2(void **ptr, size_t size, char const *name)
{
    void *new_ptr = mdv_realloc(*ptr, size, name);
    if (new_ptr)
        *ptr = new_ptr;
    return new_ptr;
}


void mdv_free(void *ptr, char const *name)
{
    if (ptr)
    {
        rpfree(ptr);
        MDV_DEALLOCATION(free, ptr, name);
    }
}


void *mdv_stalloc(size_t alignment, size_t size, char const *name)
{
    if (size + alignment < mdv_stack_free_space(_thread_local_buff))
    {
        void *ptr = _thread_local_buff.data + _thread_local_buff.size;

        size_t align = (size_t)ptr % alignment;

        if (align)
            align = alignment - align;

        ptr += align;

        MDV_LOGD("stalloc(%p:%zu) '%s'", ptr, size, name);

        _thread_local_buff.size += size + align;

        return ptr;
    }

    MDV_LOGW("Thread local buffer is busy. Dynamic allocation is performed.");

    return mdv_aligned_alloc(alignment, size, name);
}


void mdv_stfree(void *ptr, char const *name)
{
    if (ptr)
    {
        MDV_LOGD("stfree(%p) '%s'", ptr, name);

        if ((uint8_t*)ptr >= _thread_local_buff.data
            && (uint8_t*)ptr < _thread_local_buff.data + _thread_local_buff.size)
        {
            _thread_local_buff.size = (uint8_t*)ptr - _thread_local_buff.data;
        }
        else
            mdv_free(ptr, name);
    }
}
