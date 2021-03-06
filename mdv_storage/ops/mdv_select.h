/**
 * @file mdv_select.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Operation for DB entries filtering using some predicate
 * @version 0.1
 * @date 2021-03-06
 *
 * @copyright Copyright (c) 2021, Vladislav Volkov
 *
 */
#pragma once
#include <ops/mdv_op.h>
#include <mdv_predicate.h>


/**
 * @brief Create DB entries filter
 *
 * @param src [in] Source operation
 *
 * @return DB entries filter
 */
mdv_op * mdv_select(mdv_op *src, mdv_predicate *predicate);
