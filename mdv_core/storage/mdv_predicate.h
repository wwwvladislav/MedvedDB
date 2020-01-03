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


/**
 * @brief Creates predicate.
 * @details Espression is parsed and compiled for VM
 */
mdv_vector * mdv_predicate_parse(char const *expression);
