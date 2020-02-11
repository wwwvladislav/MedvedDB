/**
 * @file mdv_sha3.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief SHA3 hash calculation
 * @version 0.1
 * @date 2020-02-11
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


enum
{
    MDV_SHA3_HASH_SIZE = 32,        /// SHA3 hash size
};


/**
 * @brief SHA3 hash calculation
 *
 * @param data [in]  data for hash calculation
 * @param size [in]  data size
 * @param hash [out] calculated hash
 *
 * @return On success returns true
 * @return On error returns false
 */
bool mdv_sha3(void const *data, size_t size, uint8_t hash[MDV_SHA3_HASH_SIZE]);
