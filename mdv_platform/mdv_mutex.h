/**
 * @file
 * @brief Mutex
 */
#pragma once
#include "mdv_def.h"

#ifdef MDV_PLATFORM_LINUX
    #include <pthread.h>
#endif


/// Mutex descriptor
typedef pthread_mutex_t mdv_mutex;


/**
 * @brief Create new mutex
 *
 * @param mutex [in]   mutex
 *
 * @return MDV_OK if mutex is suzeccesfully created
 * @return non zero error code if error occurred
 */
mdv_errno mdv_mutex_create(mdv_mutex *mutex);


/**
 * @brief Free mutex
 *
 * @param mutex [in]   mutex
 */
void mdv_mutex_free(mdv_mutex *mutex);


/**
 * @brief Lock a mutex
 *
 * @param mutex [in] mutex
 *
 * @return MDV_OK mutex is successfully locked
 * @return non zero value if error has occurred
 */
mdv_errno mdv_mutex_lock(mdv_mutex *mutex);


/**
 * @brief Try to lock a mutex
 *
 * @param mutex [in] mutex
 *
 * @return MDV_OK mutex is successfully locked
 * @return non zero value if error has occurred
 */
mdv_errno mdv_mutex_trylock(mdv_mutex *mutex);


/**
 * @brief Unlock a mutex
 *
 * @param mutex [in] mutex
 *
 * @return MDV_OK mutex is successfully unlocked
 * @return non zero value if error has occurred
 */
mdv_errno mdv_mutex_unlock(mdv_mutex *mutex);

