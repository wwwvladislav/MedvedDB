#pragma once
#include "../minunit.h"
#include <mdv_bloom_filter.h>


MU_TEST(platform_bloom_filter)
{
    size_t const size = MDV_BLOOM_FILTER_SIZE(1000000000, 0.99);
    size_t const hash_fns = MDV_BLOOM_FILTER_HASH_FNS(size, 1000000000);


    int i = 0;
    ++i;
//    mu_check(mdv_mkdir("./ololo/alala/atata"));
//    mu_check(mdv_mkdir("./ololo/alala/azaza"));
//    mu_check(mdv_rmdir("./ololo"));
}



//mdv_bloom_filter