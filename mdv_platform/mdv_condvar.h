/**
 * @file
 * @brief Condition variable
 */
#pragma once
#include "mdv_def.h"
#include <stddef.h>

#ifdef MDV_PLATFORM_LINUX
    #include <pthread.h>
#endif


/// Condition variable descriptor
typedef struct mdv_condvar
{
    pthread_mutex_t mutex;      ///< Mutex
    pthread_cond_t  cv;         ///< Conditional variable
} mdv_condvar;


/**
 * @brief Create new condition variable
 *
 * @param cv [out]   condition variable
 *
 * @return On success return MDV_OK
 * @return On error non zero value is returned
 */
mdv_errno mdv_condvar_create(mdv_condvar *cv);


/**
 * @brief Free condition variable
 *
 * @param cv [in]   condition variable
 */
void mdv_condvar_free(mdv_condvar *cv);


/**
 * @brief Signal a condition
 *
 * @param cv [in] condition variable
 *
 * @return On success return MDV_OK
 * @return On error non zero value is returned
 */
mdv_errno mdv_condvar_signal(mdv_condvar *cv);


/**
 * @brief Wait on a condition
 *
 * @param cv [in] condition variable
 *
 * @return On success return MDV_OK
 * @return On error non zero value is returned
 */
mdv_errno mdv_condvar_wait(mdv_condvar *cv);


/**
 * @brief Wait on a condition
 *
 * @param cv [in] condition variable
 * @param duration [in] time duration in milliseconds for wait
 *
 * @return On success return MDV_OK or MDV_ETIMEDOUT
 * @return On error non zero value is returned
 */
mdv_errno mdv_condvar_timedwait(mdv_condvar *cv, size_t duration);


