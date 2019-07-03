/**
 * @file
 * @brief Mutex
 */
#pragma once
#include "mdv_errno.h"


/// Mutex descriptor
typedef struct mdv_mutex mdv_mutex;


/**
 * @brief Create new mutex
 *
 * @return On success, return new mutex
 * @return On error, return NULL
 */
mdv_mutex * mdv_mutex_create();


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

