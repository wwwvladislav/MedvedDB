#pragma once
#include <math.h>
#include <stddef.h>


#define MDV_LN2 0.69314718055994530942	// log_e 2

#define MDV_BLOOM_FILTER_SIZE(N, P)     (size_t)(-N * log(P) / (MDV_LN2 * MDV_LN2))

#define MDV_BLOOM_FILTER_HASH_FNS(M, N) (size_t)(MDV_LN2 * M / N)

