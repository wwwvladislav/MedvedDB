/**
 * @file
 * @brief Mutex
 */
#pragma once
#include "mdv_errno.h"
#include "mdv_def.h"

#ifdef MDV_PLATFORM_LINUX
    #include <pthread.h>
#endif


/// Mutex descriptor
typedef pthread_mutex_t mdv_mutex;


/**
 * @brief Create new mutex
 *
 * @param m [in]   mutex
 *
 * @return MDV_OK if mutex is suzeccesfully created
 * @return non zero error code if error occurred
 */
mdv_errno mdv_mutex_create(mdv_mutex *m);


/**
 * @brief Free mutex
 *
 * @param m [in]   mutex
 */
void mdv_mutex_free(mdv_mutex *m);


/**
 * @brief Lock a mutex
 *
 * @param m [in] mutex
 *
 * @return MDV_OK mutex is successfully locked
 * @return non zero value if error has occurred
 */
mdv_errno mdv_mutex_lock(mdv_mutex *m);


/**
 * @brief Try to lock a mutex
 *
 * @param m [in] mutex
 *
 * @return MDV_OK mutex is successfully locked
 * @return non zero value if error has occurred
 */
mdv_errno mdv_mutex_trylock(mdv_mutex *m);


/**
 * @brief Unlock a mutex
 *
 * @param m [in] mutex
 *
 * @return MDV_OK mutex is successfully unlocked
 * @return non zero value if error has occurred
 */
mdv_errno mdv_mutex_unlock(mdv_mutex *m);

