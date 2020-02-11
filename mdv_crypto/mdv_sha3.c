#include "mdv_sha3.h"
#include <sha3.h>


bool mdv_sha3(void const *data, size_t size, uint8_t hash[MDV_SHA3_HASH_SIZE])
{
    sha3(data, size, hash, MDV_SHA3_HASH_SIZE);
    return true;
}

