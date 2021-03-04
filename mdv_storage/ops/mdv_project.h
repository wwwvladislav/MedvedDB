/**
 * @file mdv_project.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Operation for subset extraction from the DB entry
 * @version 0.1
 * @date 2021-03-03
 *
 * @copyright Copyright (c) 2021, Vladislav Volkov
 *
 */
#pragma once
#include <ops/mdv_op.h>


/**
 * @brief Create range projection operation
 * @details The range [from, to) is used for projection.
 *
 * @param src [in] Source operation
 * @param from [in] Range start
 * @param to [in] Range end
 *
 * @return projection operation
 */
mdv_op * mdv_project_range(mdv_op *src, size_t from, size_t to);


/**
 * @brief Create projection operation by indices
 *
 * @param src [in] Source operation
 * @param size [in] Indices array size
 * @param indices [in] Indices for projection
 *
 * @return projection operation
 */
mdv_op * mdv_project_by_indices(mdv_op *src, size_t size, size_t const *indices);
