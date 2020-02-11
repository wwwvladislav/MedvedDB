#include "mdv_ecc.h"
#include "mdv_rand.h"


int mdv_ecc_generate_keys(uint8_t public_key[MDV_ECC_KEY_SIZE], uint8_t private_key[MDV_ECC_KEY_SIZE])
{
    mdv_random(private_key, MDV_ECC_KEY_SIZE);
    // TODO
    return 0;
}
