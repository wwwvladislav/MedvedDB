#pragma once
#include "../minunit.h"
#include <mdv_bloom.h>


MU_TEST(platform_bloom)
{
    mdv_bloom * bloom = mdv_bloom_create(10000, 0.01);

    mdv_bloom_free(bloom);
//    mu_check(mdv_rmdir("./ololo"));
}
