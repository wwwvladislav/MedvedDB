#include "mdv_bloom.h"
#include "mdv_alloc.h"
#include "mdv_log.h"
#include <limits.h>
#include <string.h>


#define MDV_LN2 0.69314718055994530942	// log_e 2

#define MDV_BLOOM_BITS(entries, err)        (size_t)(1 - entries * log(err) / (MDV_LN2 * MDV_LN2))
#define MDV_BLOOM_BYTES(entries, err)       ((MDV_BLOOM_BITS(entries, err) + 7) / 8)
#define MDV_BLOOM_HASH_FNS(bits, entries)   (size_t)(1 + MDV_LN2 * bits / entries)


struct mdv_bloom_s
{
    uint32_t entries;   // number of entries
    uint32_t added;     // actually added number of entries
    uint32_t hashes;    // hash functions number
    double   err;       // error
    uint8_t  bits[1];   // bloom filter
};


mdv_bloom * mdv_bloom_create(uint32_t entries, double err)
{
    size_t const bits = MDV_BLOOM_BITS(entries, err);
    size_t const bytes = MDV_BLOOM_BYTES(entries, err);
    size_t const size = offsetof(mdv_bloom, bits) + bytes;

    mdv_bloom *bloom = (mdv_bloom *)mdv_alloc(size);

    if (!bloom)
    {
        MDV_LOGE("bloom_create failed. No memory.");
        return 0;
    }

    bloom->entries = entries;
    bloom->added = 0;
    bloom->hashes = MDV_BLOOM_HASH_FNS(bits, entries);
    bloom->err = err > 1.0 ? 1.0
                 : err < 0.00001 ? 0.00001
                 : err;
    memset(bloom->bits, 0, bytes);

    return bloom;
}


void mdv_bloom_free(mdv_bloom *bloom)
{
    mdv_free(bloom);
}

