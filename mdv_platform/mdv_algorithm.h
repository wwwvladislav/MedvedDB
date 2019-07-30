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
#include "mdv_def.h"


/**
 * @brief Calculate union and difference for two sorted arrays
 * 
 * @param set_a [in]        First array sorted in ascending order
 * @param a_size [in]       First array size
 * @param set_b [in]        Second array sorted in ascending order
 * @param b_size [in]       Second array size
 * @param sets_union [out]  Place for arrays union
 * @param union_size [out]  Arrays union size
 * @param sets_diff [out]   Place for arrays difference
 * @param diff_size [out]   Arrays difference size
 */
void mdv_union_and_diff_u32(uint32_t const *set_a, uint32_t a_size,
                            uint32_t const *set_b, uint32_t b_size,
                            uint32_t *sets_union, uint32_t *union_size,
                            uint32_t *sets_diff, uint32_t *diff_size);
