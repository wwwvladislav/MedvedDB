/**
 * @file
 * @brief Universally Unique IDentifier
 */

#pragma once
#include "mdv_string.h"
#include "mdv_def.h"


/// Universally Unique IDentifier
typedef struct
{
    union
    {
        uint8_t  u8[16];    ///< UUID representation as u8 array
        uint64_t u64[2];    ///< UUID representation as u64 array

        struct
        {
            int32_t a;
            int16_t b;
            int16_t c;
            int16_t d;
            int16_t e;
            int16_t f;
            int16_t g;
        };
    };
} mdv_uuid;


/**
 * @brief New UUID generation
 *
 * @return New randomly generated UUID
 */
mdv_uuid mdv_uuid_generate();


/**
 * @brief Two UUIDs comparision
 *
 * @param a [in]    first UUID
 * @param b [in]    second UUID
 *
 * @return an integer less than zero if a is less then b
 * @return zero if a is equal to b
 * @return an integer greater than zero if a is greater then b
 */
int mdv_uuid_cmp(mdv_uuid const *a, mdv_uuid const *b);


/**
 * @brief Hash function for UUIDs
 *
 * @param a [in]    UUID
 *
 * @return hash value for UUID
 */
size_t mdv_uuid_hash(mdv_uuid const *a);


/**
 * @brief Convert UUID to string
 *
 * @param uuid [in] UUID
 *
 * @return string representation of UUID
 */
mdv_string mdv_uuid_to_str(mdv_uuid const *uuid);


/**
 * @brief Convert string to UUID
 *
 * @param uuid [out]    UUID
 * @param str [in]      string representation of UUID
 *
 * @return On success, return true and UUID is placed to UUID pointer
 * @return On error, return false
 */
bool mdv_uuid_from_str(mdv_uuid *uuid, mdv_string const *str);
