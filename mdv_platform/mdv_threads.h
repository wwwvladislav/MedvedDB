/**
 * @file
 * @brief API for multithreaded application development.
 */
#pragma once
#include "mdv_errno.h"
#include <stddef.h>


/// Thread descriptor
typedef void* mdv_thread;


/**
 * @brief Suspends execution of the calling thread for msec milliseconds.
 *
 * @param msec [in] time duration for wait in milliseconds
 */
void mdv_sleep(size_t msec);


/// Thread entry point.
typedef void *(*mdv_thread_fn) (void *);


enum
{
    MDV_THREAD_STACK_SIZE = 1 * 1024 * 1024     ///< default stack size for thread
};


/// Attributes for a new thread
typedef struct mdv_thread_attrs
{
    size_t stack_size;      ///< stack size
} mdv_thread_attrs;


/**
 * @brief Start a new thread in the calling process.
 *
 * @param thread [out]  pointer for a new thread ID
 * @param attrs [in]    attributes for the new thread
 * @param fn [in]       thread function
 * @param arg [in]      argument which is passed to thread function
 *
 * @return MDV_OK if new thread is successfully started. The thread pointer should contain new thread descriptor.
 * @return non zero value if error has occurred
 */
mdv_errno mdv_thread_create(mdv_thread *thread, mdv_thread_attrs const *attrs, mdv_thread_fn fn, void *arg);


/**
 * @brief Obtain ID of the calling thread.
 *
 * @return ID of the calling thread
 */
mdv_thread mdv_thread_self();


/**
 * @brief Compare thread IDs
 *
 * @param t1 [in]   first thread ID
 * @param t2 [in]   second thread ID
 *
 * @return nonzero if two thread IDs are equal
 * @return zero if two thread IDs are not equal
 */
int mdv_thread_equal(mdv_thread t1, mdv_thread t2);


/**
 * @brief Join with a terminated thread
 *
 * @param thread [in]   thread ID
 *
 * @return MDV_OK if thread is successfully joined.
 * @return non zero value if error has occurred
 */
mdv_errno mdv_thread_join(mdv_thread thread);

