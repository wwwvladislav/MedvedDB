/**
 * @file mdv_rand.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Random numbers generator
 * @version 0.1
 * @date 2020-02-11
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include <stdint.h>
#include <stddef.h>


/**
 * @brief Random data generation
 *
 * @param buf [out] desination buffer for random data
 * @param size [in] buffer size
 */
void mdv_random(uint8_t *buf, size_t size);

