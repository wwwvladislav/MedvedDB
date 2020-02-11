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
#include <stdbool.h>


enum
{
    MDV_ECC_PRIVATE_KEY_SIZE            = 32,   /// Private key size
    MDV_ECC_PUBLIC_KEY_SIZE             = 64,   /// Public key size
    MDV_ECC_PUBLIC_KEY_COMPRESSED_SIZE  = 33,   /// Compressed public key size
    MDV_ECC_SIGNATURE_SIZE              = 64,   /// Signature size
    MDV_ECC_HASH_SIZE                   = 32,   /// Data hash size for signature calculation
    MDV_ECC_SHARED_SECRET_SIZE          = 32,   /// Shared secret size
};


/**
 * @brief Private and public key pair generation
 *
 * @param public [out]  Public key
 * @param private [out] Private key
 *
 * @return On success returns true
 * @return On error returns false
 */
bool mdv_ecc_generate_keys(uint8_t public[MDV_ECC_PUBLIC_KEY_SIZE],
                           uint8_t private[MDV_ECC_PRIVATE_KEY_SIZE]);


/**
 * @brief Public key compression
 *
 * @param public [in]       Public key
 * @param compressed [out]  Compressed public key
 */
void mdv_ecc_compress(uint8_t const public[MDV_ECC_PUBLIC_KEY_SIZE],
                      uint8_t       compressed[MDV_ECC_PUBLIC_KEY_COMPRESSED_SIZE]);


/**
 * @brief Public key decompression
 *
 * @param compressed [in]   Compressed public key
 * @param public [out]      Public key
 */
void mdv_ecc_decompress(uint8_t const compressed[MDV_ECC_PUBLIC_KEY_COMPRESSED_SIZE],
                        uint8_t       public[MDV_ECC_PUBLIC_KEY_SIZE]);


/**
 * @brief Check to see if a public key is valid.
 *
 * @param public [in]       Public key
 *
 * @return true if the public key is valid
 * @return false if it is invalid
 */
bool mdv_ecc_valid_public_key(uint8_t const public[MDV_ECC_PUBLIC_KEY_SIZE]);


/**
 * @brief Data signature calculation
 *
 * @param private [in]      Private key
 * @param hash [in]         Data hash
 * @param signature [out]   Signature
 *
 * @return On success returns true
 * @return On error returns false
 */
bool mdv_ecc_sign(uint8_t const private[MDV_ECC_PRIVATE_KEY_SIZE],
                  uint8_t const hash[MDV_ECC_HASH_SIZE],
                  uint8_t       signature[MDV_ECC_SIGNATURE_SIZE]);


/**
 * @brief Data signature verification
 *
 * @param public [in]       Public key
 * @param hash [in]         Data hash
 * @param signature [in]    Signature
 *
 * @return On success returns true
 * @return On error returns false
 */
bool mdv_ecc_verify(uint8_t const public[MDV_ECC_PUBLIC_KEY_SIZE],
                    uint8_t const hash[MDV_ECC_HASH_SIZE],
                    uint8_t const signature[MDV_ECC_SIGNATURE_SIZE]);


/**
 * @brief Shared secret calculation
 *
 * @param public [in]   The public key of the remote party
 * @param private [in]  Private key
 * @param secret [out]  Shared secret value
 *
 * @return On success returns true
 * @return On error returns false
 */
bool mdv_ecc_shared_secret(uint8_t const public[MDV_ECC_PUBLIC_KEY_SIZE],
                           uint8_t const private[MDV_ECC_PRIVATE_KEY_SIZE],
                           uint8_t       secret[MDV_ECC_SHARED_SECRET_SIZE]);
