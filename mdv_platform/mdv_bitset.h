/**
 * @file mdv_bitset.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Fixed-size sequence of N bits.
 * @version 0.1
 * @date 2019-10-13
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_alloc.h"


/// Fixed-size sequence of N bits
typedef struct mdv_bitset mdv_bitset;


/**
 * @brief Creates new bitset given capacity
 */
mdv_bitset * mdv_bitset_create(size_t size, mdv_allocator const *allocator);


/**
 * @brief Deallocates bitset created by mdv_bitset_create()
 */
void mdv_bitset_free(mdv_bitset *bitset);


/**
 * @brief Sets the bit at position pos to the value true.
 */
bool mdv_bitset_set(mdv_bitset *bitset, size_t pos);


/**
 * @brief Sets the bit at position pos to the value false.
 */
void mdv_bitset_reset(mdv_bitset *bitset, size_t pos);


/**
 * @brief Returns the value of the bit at the position pos.
 */
bool mdv_bitset_test(mdv_bitset const *bitset, size_t pos);


/**
 * @brief Returns the bitset size
 */
size_t mdv_bitset_size(mdv_bitset const *bitset);
