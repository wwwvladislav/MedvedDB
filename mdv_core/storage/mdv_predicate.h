#pragma once
#include <stdint.h>


/// Any function which returns true or false
typedef struct mdv_predicate mdv_predicate;


typedef mdv_predicate *  (*mdv_predicate_retain_fn) (mdv_predicate *);
typedef uint32_t         (*mdv_predicate_release_fn)(mdv_predicate *);


/// Interface for row sets
struct mdv_predicate
{
    mdv_predicate_retain_fn     retain;        ///< Function for predicate retain
    mdv_predicate_release_fn    release;       ///< Function for predicate release
};


/**
 * @brief Retains predicate.
 * @details Reference counter is increased by one.
 */
mdv_predicate * mdv_predicate_retain(mdv_predicate *predicate);


/**
 * @brief Releases predicate.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the predicate is freed.
 */
uint32_t mdv_predicate_release(mdv_predicate *predicate);
