/**
 * @file mdv_ecc.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Cryptographic library based on Elliptic-Curves
 * @version 0.1
 * @date 2020-02-11
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include <stdint.h>


enum
{
    MDV_ECC_KEY_SIZE = 32,                      /// Key size
    MDV_ECC_SIG_SIZE = MDV_ECC_KEY_SIZE * 2,    /// Signature size
};


/**
 * @brief Private and public key pair generation
 *
 * @param public_key [out]
 * @param private_key [out]
 *
 * @return On success returns 0
 * @return On error returns -1
 */
int mdv_ecc_generate_keys(uint8_t public_key[2 * MDV_ECC_KEY_SIZE], uint8_t private_key[MDV_ECC_KEY_SIZE]);
