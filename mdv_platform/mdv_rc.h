/**
 * @file mdv_rc.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Reference-counting pointer.
 * @version 0.1
 * @date 2019-09-25
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_def.h"


/// Reference-counting pointer
typedef struct mdv_rc mdv_rc;


/**
 * @brief Create new RC object
 *
 * @param size [in]         memory size required for object
 * @param arg [in]          argument passed to constructor
 * @param constructor [in]  object constructor
 * @param destructor [in]   object destructor
 *
 * @return Reference-counting pointer
 */
mdv_rc * mdv_rc_create(size_t size,
                       void *arg,
                       bool (*constructor)(void *arg, void *ptr),
                       void (*destructor)(void *ptr));


/**
 * @brief Retains reference-counting pointer.
 * @details Reference counter is increased by one.
 *
 * @param rc [in] Reference-counting pointer
 *
 * @return Reference-counting pointer
 */
mdv_rc * mdv_rc_retain(mdv_rc *rc);


/**
 * @brief Releases reference-counting pointer.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the object's destructor is called.
 *
 * @param rc [in] Reference-counting pointer
 */
void mdv_rc_release(mdv_rc *rc);


/**
 * @brief Returns object pointer
 *
 * @param rc [in] Reference-counting pointer
 *
 * @return object pointer
 */
void * mdv_rc_ptr(mdv_rc *rc);
