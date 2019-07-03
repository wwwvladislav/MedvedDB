#include "mdv_threadpool.h"
#include "mdv_queuefd.h"
#include "mdv_threads.h"
#include "mdv_alloc.h"


/// @cond Doxygen_Suppress

struct mdv_threadpool
{
    size_t      size;
    mdv_thread *threads;
};

/// @endcond


mdv_threadpool * mdv_threadpool_create(size_t size)
{
    //offsetof
}


void mdv_threadpool_free(mdv_threadpool *threadpool)
{}


