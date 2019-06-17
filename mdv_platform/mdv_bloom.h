#pragma once
#include <math.h>
#include <stddef.h>


#define MDV_LN2 0.69314718055994530942	// log_e 2

#define MDV_BLOOM_BITS(entries, err)        (size_t)(1 - entries * log(err) / (MDV_LN2 * MDV_LN2))
#define MDV_BLOOM_BYTES(entries, err)       ((MDV_BLOOM_BITS(entries, err) + 7) / 8)
#define MDV_BLOOM_HASH_FNS(bits, entries)   (size_t)(1 + MDV_LN2 * bits / entries)


typedef struct
{
} mdv_bloom;
