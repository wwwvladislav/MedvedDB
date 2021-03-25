/**
 * @file mdv_functional.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief This header provides the standard function declarations.
 * @version 0.1
 * @date 2021-03-25
 *
 * @copyright Copyright (c) 2021, Vladislav Volkov
 *
 */
#pragma once
#include <stddef.h>


/// Hash function type
typedef size_t (*mdv_hash_fn)(void const *);


/// Comparison function type
typedef int (*mdv_cmp_fn)(void const *, void const *);

