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
#include <mdv_alloc.h>


/// Fixed-size sequence of N bits
typedef struct mdv_bitset mdv_bitset;


/**
 * @brief Creates new bitset given capacity
 */
mdv_bitset * mdv_bitset_create(size_t size, mdv_allocator const *allocator);


/**
 * @brief Retains bitset.
 * @details Reference counter is increased by one.
 */
mdv_bitset * mdv_bitset_retain(mdv_bitset *bitset);


/**
 * @brief Releases bitset.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the bitset's destructor is called.
 */
uint32_t mdv_bitset_release(mdv_bitset *bitset);


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
 * @brief Fills bitset with given value.
 */
void mdv_bitset_fill(mdv_bitset *bitset, bool val);


/**
 * @brief Returns the bitset size
 */
size_t mdv_bitset_size(mdv_bitset const *bitset);


/**
 * @brief Returns items number in bitset
 */
size_t mdv_bitset_count(mdv_bitset const *bitset, bool val);


/**
 * @brief Returns the bitset capacity (in bits number). Bitset alignment is 32 bits.
 */
size_t mdv_bitset_capacity(mdv_bitset const *bitset);


enum { MDV_BITSET_ALIGNMENT = sizeof(uint32_t) };


/**
 * @brief Returns pointer to the underlying array serving as element storage.
 */
uint32_t const * mdv_bitset_data(mdv_bitset const *bitset);
