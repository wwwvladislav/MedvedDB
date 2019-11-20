/**
 * @file mdv_enumerator.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Iterator interface definition
 * @version 0.1
 * @date 2019-11-20
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_def.h"
#include <stdatomic.h>


/// Iterator for set of rows
typedef struct mdv_enumerator mdv_enumerator;


typedef mdv_enumerator * (*mdv_enumerator_retain_fn) (mdv_enumerator *);
typedef uint32_t         (*mdv_enumerator_release_fn)(mdv_enumerator *);
typedef mdv_errno        (*mdv_enumerator_reset_fn)  (mdv_enumerator *);
typedef mdv_errno        (*mdv_enumerator_next_fn)   (mdv_enumerator *);
typedef void *           (*mdv_enumerator_current_fn)(mdv_enumerator *);


/// Interface for iterators
typedef struct
{
    mdv_enumerator_retain_fn    retain;             ///< Function for iterator retain
    mdv_enumerator_release_fn   release;            ///< Function for iterator release
    mdv_enumerator_reset_fn     reset;              ///< Function for iterator reset
    mdv_enumerator_next_fn      next;               ///< Function for next iterable entry obtain
    mdv_enumerator_current_fn   current;            ///< Function for current iterable entry obtain
} mdv_ienumerator;


struct mdv_enumerator
{
    mdv_ienumerator const      *vptr;               ///< Interface for iterator
    atomic_uint_fast32_t        rc;                 ///< References counter
};


mdv_enumerator * mdv_enumerator_retain(mdv_enumerator *enumerator);
uint32_t         mdv_enumerator_release(mdv_enumerator *enumerator);
mdv_errno        mdv_enumerator_reset(mdv_enumerator *enumerator);
mdv_errno        mdv_enumerator_next(mdv_enumerator *enumerator);
void *           mdv_enumerator_current(mdv_enumerator *enumerator);
