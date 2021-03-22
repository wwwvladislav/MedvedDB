/**
 * @file mdv_algorithm.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief The algorithms module defines functions for a variety of purposes.
 * @version 0.1
 * @date 2019-07-30
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_def.h>


/**
 * @brief Calculate difference for two sorted arrays
 *
 * @param set_a [in]        First array sorted in ascending order
 * @param a_size [in]       First array size
 * @param set_b [in]        Second array sorted in ascending order
 * @param b_size [in]       Second array size
 * @param sets_diff [out]   Place for arrays difference
 * @param diff_size [out]   Arrays difference size
 */
void mdv_diff_u32(uint32_t const *set_a, uint32_t a_size,
                  uint32_t const *set_b, uint32_t b_size,
                  uint32_t *sets_diff, uint32_t *diff_size);


/**
 * @brief Calculate union for two sorted arrays
 *
 * @param set_a [in]        First array sorted in ascending order
 * @param a_size [in]       First array size
 * @param set_b [in]        Second array sorted in ascending order
 * @param b_size [in]       Second array size
 * @param sets_union [out]  Place for arrays union
 * @param union_size [out]  Arrays union size
 */
void mdv_union_u32(uint32_t const *set_a, uint32_t a_size,
                   uint32_t const *set_b, uint32_t b_size,
                   uint32_t *sets_union, uint32_t *union_size);


/**
 * @brief Calculate difference for two sorted arrays. The result is placed to the first array.
 *
 * @param set_a [in] [out]  First array sorted in ascending order
 * @param itmsize_a [in]    First array entry size
 * @param size_a [in]       First array size
 * @param set_b [in]        Second array sorted in ascending order
 * @param itmsize_b [in]    Second array entry size
 * @param size_b [in]       Second array size
 * @param cmp [in]          Comparison function
 *
 * @return result array size
 */
uint32_t mdv_exclude(void       *set_a, size_t itmsize_a, size_t size_a,
                     void const *set_b, size_t itmsize_b, size_t size_b,
                     int (*cmp)(const void *a, const void *b));


/**
 * @brief Swaps the values a and b.
 *
 * @param type [in] Value type
 * @param a [in]    First value
 * @param b [in]    Second value
 */
#define mdv_swap(type, a, b) { type mdvtmp_85628935 = a; a = b; b = mdvtmp_85628935; }
