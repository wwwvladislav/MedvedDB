#pragma once
#include "../minunit.h"
#include <mdv_bloom.h>


MU_TEST(platform_bloom)
{
    size_t const bits = MDV_BLOOM_BITS(1000000000, 0.01);
    size_t const bytes = MDV_BLOOM_BYTES(1000000000, 0.01);
    size_t const hash_fns = MDV_BLOOM_HASH_FNS(bits, 1000000000);


    int i = 0;
    ++i;
//    mu_check(mdv_rmdir("./ololo"));
}



//mdv_bloom_filter