#include "mdv_bloom.h"
#include "mdv_hash.h"
#include "mdv_alloc.h"
#include <mdv_log.h>
#include <limits.h>
#include <string.h>
#include <stddef.h>


#define MDV_LN2 0.69314718055994530942	// log_e 2

#define MDV_BLOOM_BITS(entries, err)        (size_t)(1 - entries * log(err) / (MDV_LN2 * MDV_LN2))
#define MDV_BLOOM_BYTES(bits)               ((bits + 7) / 8)
#define MDV_BLOOM_HASH_FNS(bits, entries)   (size_t)(1 + MDV_LN2 * bits / entries)


static const uint32_t MDV_HASH_SEED = 836091947u;


/// Bloom filter
struct mdv_bloom
{
    size_t   bits;      ///< number of bits in bloom filter
    uint32_t capacity;  ///< max entries number
    uint32_t size;      ///< actually added number of entries
    uint32_t hashes;    ///< hash functions number
    double   err;       ///< error
    uint8_t  bf[1];     ///< bloom filter
};


mdv_bloom * mdv_bloom_create(uint32_t entries, double err)
{
    size_t const bits = MDV_BLOOM_BITS(entries, err);
    size_t const bytes = MDV_BLOOM_BYTES(bits);
    size_t const size = offsetof(mdv_bloom, bf) + bytes;

    mdv_bloom *bloom = (mdv_bloom *)mdv_alloc(size);

    if (!bloom)
    {
        MDV_LOGE("bloom_create failed. No memory.");
        return 0;
    }

    bloom->bits = bits;
    bloom->capacity = entries;
    bloom->size = 0;
    bloom->hashes = MDV_BLOOM_HASH_FNS(bits, entries);
    bloom->err = err > 1.0 ? 1.0
                 : err < 0.00001 ? 0.00001
                 : err;
    memset(bloom->bf, 0, bytes);

    return bloom;
}


void mdv_bloom_free(mdv_bloom *bloom)
{
    mdv_free(bloom);
}


bool mdv_bloom_insert(mdv_bloom *bloom, void const *data, uint32_t const len)
{
    if (bloom->size >= bloom->capacity)
        return false;

    bool is_changed = false;

    for(uint32_t i = 0; i < bloom->hashes; ++i)
    {
        uint32_t const hash = mdv_hash_murmur2a(data, len, i * MDV_HASH_SEED);
        uint32_t const bit = hash % bloom->bits;
        uint32_t const byte = bit / 8;
        uint32_t const offset = bit % 8;
        is_changed |= (bloom->bf[byte] >> offset) == 0;
        bloom->bf[byte] |= 1 << offset;
    }

    if (is_changed)
        bloom->size++;

    return is_changed;
}


bool mdv_bloom_contains(mdv_bloom *bloom, void const *data, uint32_t const len)
{
    for(uint32_t i = 0; i < bloom->hashes; ++i)
    {
        uint32_t const hash = mdv_hash_murmur2a(data, len, i * MDV_HASH_SEED);
        uint32_t const bit = hash % bloom->bits;
        uint32_t const byte = bit / 8;
        uint32_t const offset = bit % 8;
        if ((bloom->bf[byte] >> offset) == 0)
            return false;
    }

    return true;
}


uint32_t mdv_bloom_capacity(mdv_bloom const *bloom)
{
    return bloom->capacity;
}


uint32_t mdv_bloom_size(mdv_bloom const *bloom)
{
    return bloom->size;
}


#undef MDV_LN2
#undef MDV_BLOOM_BITS
#undef MDV_BLOOM_BYTES
#undef MDV_BLOOM_HASH_FNS
