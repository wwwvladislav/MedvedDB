#include "mdv_ecc.h"
#include <uECC.h>


bool mdv_ecc_generate_keys(uint8_t public[MDV_ECC_PUBLIC_KEY_SIZE],
                           uint8_t private[MDV_ECC_PRIVATE_KEY_SIZE])
{
    return !!uECC_make_key(public, private, uECC_secp256k1());
}


void mdv_ecc_compress(uint8_t const public[MDV_ECC_PUBLIC_KEY_SIZE],
                      uint8_t       compressed[MDV_ECC_PUBLIC_KEY_COMPRESSED_SIZE])
{
    uECC_compress(public, compressed, uECC_secp256k1());
}


void mdv_ecc_decompress(uint8_t const compressed[MDV_ECC_PUBLIC_KEY_COMPRESSED_SIZE],
                        uint8_t       public[MDV_ECC_PUBLIC_KEY_SIZE])
{
    uECC_decompress(compressed, public, uECC_secp256k1());
}


bool mdv_ecc_valid_public_key(uint8_t const public[MDV_ECC_PUBLIC_KEY_SIZE])
{
    return !!uECC_valid_public_key(public, uECC_secp256k1());
}


bool mdv_ecc_sign(uint8_t const private[MDV_ECC_PRIVATE_KEY_SIZE],
                  uint8_t const hash[MDV_ECC_HASH_SIZE],
                  uint8_t       signature[MDV_ECC_SIGNATURE_SIZE])
{
    return !!uECC_sign(private, hash, MDV_ECC_HASH_SIZE, signature, uECC_secp256k1());
}


bool mdv_ecc_verify(uint8_t const public[MDV_ECC_PUBLIC_KEY_SIZE],
                    uint8_t const hash[MDV_ECC_HASH_SIZE],
                    uint8_t const signature[MDV_ECC_SIGNATURE_SIZE])
{
    return !!uECC_verify(public, hash, MDV_ECC_HASH_SIZE, signature, uECC_secp256k1());
}


bool mdv_ecc_shared_secret(uint8_t const public[MDV_ECC_PUBLIC_KEY_SIZE],
                           uint8_t const private[MDV_ECC_PRIVATE_KEY_SIZE],
                           uint8_t       secret[MDV_ECC_SHARED_SECRET_SIZE])
{
    return !!uECC_shared_secret(public, private, secret, uECC_secp256k1());
}
