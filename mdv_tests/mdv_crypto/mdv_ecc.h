#pragma once
#include "../minunit.h"
#include <mdv_ecc.h>


MU_TEST(crypto_ecc)
{
    uint8_t public_key[MDV_ECC_KEY_SIZE] = {};
    uint8_t private_key[MDV_ECC_KEY_SIZE] = {};

    mu_check(mdv_ecc_generate_keys(public_key, private_key) == 0);
}
