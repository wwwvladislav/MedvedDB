/**
 * @file mdv_predicate.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Predicate parser. Predicate is any function which returns true or false.
 * @version 0.1
 * @date 2020-01-03, Vladislav Volkov
 * @copyright Copyright (c) 2020
 */
#pragma once
#include <mdv_vector.h>
#include <mdv_vm.h>


/// Predicate
typedef struct mdv_predicate mdv_predicate;


/**
 * @brief Creates predicate.
 * @details Expression is parsed and compiled for VM
 */
mdv_predicate * mdv_predicate_parse(char const *expression);


/**
 * @brief Retains predicate.
 * @details Reference counter is increased by one.
 */
mdv_predicate * mdv_predicate_retain(mdv_predicate *predicate);


/**
 * @brief Releases predicate.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the predicate's destructor is called.
 */
uint32_t mdv_predicate_release(mdv_predicate *predicate);


/**
 * @brief Returns compiled expression
 */
uint8_t const * mdv_predicate_expr(mdv_predicate const *predicate);


/**
 * @brief Returns custom handlers assigned with predicate
 */
mdv_vm_fn const * mdv_predicate_fns(mdv_predicate const *predicate);


/**
 * @brief Returns custom handlers count assigned with predicate
 */
size_t mdv_predicate_fns_count(mdv_predicate const *predicate);
