#pragma once
#include <minunit.h>
#include <mdv_ecc.h>
#include <mdv_sha3.h>


MU_TEST(crypto_ecc)
{
    uint8_t public_key[MDV_ECC_PUBLIC_KEY_SIZE] = {};
    uint8_t private_key[MDV_ECC_PRIVATE_KEY_SIZE] = {};
    uint8_t hash[MDV_SHA3_HASH_SIZE] = {};
    uint8_t signature[MDV_ECC_SIGNATURE_SIZE] = {};

    char const message[] = "The quick brown fox jumps over the lazy dog";

    mu_check(mdv_sha3(message, sizeof message, hash));
    mu_check(mdv_ecc_generate_keys(public_key, private_key));


    // Public key validation
    uint8_t invalid_public_key[MDV_ECC_PUBLIC_KEY_SIZE] = { 42 };

    mu_check(mdv_ecc_valid_public_key(public_key));
    mu_check(!mdv_ecc_valid_public_key(invalid_public_key));


    // Signature check
    mu_check(mdv_ecc_sign(private_key, hash, signature));
    mu_check(mdv_ecc_verify(public_key, hash, signature));


    uint8_t public_compressed_key[MDV_ECC_PUBLIC_KEY_COMPRESSED_SIZE] = {};
    uint8_t public_decompressed_key[MDV_ECC_PUBLIC_KEY_SIZE] = {};


    // Public keys compression check
    mdv_ecc_compress(public_key, public_compressed_key);
    mdv_ecc_decompress(public_compressed_key, public_decompressed_key);
    mu_check(memcmp(public_key, public_decompressed_key, MDV_ECC_PUBLIC_KEY_SIZE) == 0);


    // Shared secret calculation
    uint8_t public1[MDV_ECC_PUBLIC_KEY_SIZE] = {};
    uint8_t private1[MDV_ECC_PRIVATE_KEY_SIZE] = {};
    uint8_t public2[MDV_ECC_PUBLIC_KEY_SIZE] = {};
    uint8_t private2[MDV_ECC_PRIVATE_KEY_SIZE] = {};
    uint8_t secret1[MDV_ECC_SHARED_SECRET_SIZE] = {};
    uint8_t secret2[MDV_ECC_SHARED_SECRET_SIZE] = {};

    mu_check(mdv_ecc_generate_keys(public1, private1));
    mu_check(mdv_ecc_generate_keys(public2, private2));
    mu_check(mdv_ecc_shared_secret(public2, private1, secret1));
    mu_check(mdv_ecc_shared_secret(public1, private2, secret2));
    mu_check(memcmp(secret1, secret2, MDV_ECC_SHARED_SECRET_SIZE) == 0);
}
