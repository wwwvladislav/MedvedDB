#include "mdv_hash.h"


#define mdv_hash_murmur_mix(h, k)   \
    {                               \
        k *= m;                     \
        k ^= k >> 24;               \
        k *= m;                     \
        h *= m;                     \
        h ^= k;                     \
    }


uint32_t mdv_hash_murmur2a(void const *buff, uint32_t const len, uint32_t const seed)
{
    const uint32_t m = 0x5bd1e995;

    uint32_t l = len;
    uint32_t h = seed;

    uint8_t const *data = buff;

    while(l >= sizeof(uint32_t))
    {
        uint32_t k = *(uint32_t*)data;
        mdv_hash_murmur_mix(h, k);
        data += sizeof(uint32_t);
        l -= sizeof(uint32_t);
    }

    uint32_t t = 0;

    switch(l)
    {
        case 3: t ^= data[2] << 16;
        case 2: t ^= data[1] << 8;
        case 1: t ^= data[0];
    };

    l = len;

    mdv_hash_murmur_mix(h, t);
    mdv_hash_murmur_mix(h, l);

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

#undef mdv_hash_murmur_mix

