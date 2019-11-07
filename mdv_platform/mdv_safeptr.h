/**
 * @file mdv_safeptr.h
 * @author Vladislav Volkov (wwwVladislav@gmail.com)
 * @brief Safe pointer implementation
 * @version 0.1
 * @date 2019-11-07
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_def.h"


typedef void * (*mdv_safeptr_retain_fn)(void *);
typedef uint32_t (*mdv_safeptr_release_fn)(void *);


/// Safe pointer
typedef struct mdv_safeptr mdv_safeptr;


mdv_safeptr * mdv_safeptr_create(void *safeptr,
                                 mdv_safeptr_retain_fn retain,
                                 mdv_safeptr_release_fn release);

void mdv_safeptr_free(mdv_safeptr *ptr);

mdv_errno mdv_safeptr_set(mdv_safeptr *safeptr, void *ptr);

void * mdv_safeptr_get(mdv_safeptr *safeptr);
