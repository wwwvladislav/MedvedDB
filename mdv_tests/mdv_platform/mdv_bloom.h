#pragma once
#include "../minunit.h"
#include <mdv_bloom.h>


MU_TEST(platform_bloom)
{
    mdv_bloom *bloom = mdv_bloom_create(10, 0.01);

    mu_check(mdv_bloom_insert(bloom, "1234567890", 10));
    mu_check(mdv_bloom_insert(bloom, "1234567891", 10));
    mu_check(mdv_bloom_insert(bloom, "1234567892", 10));
    mu_check(mdv_bloom_insert(bloom, "1234567893", 10));
    mu_check(mdv_bloom_size(bloom) == 4);

    mu_check(mdv_bloom_contains(bloom, "1234567890", 10));
    mu_check(mdv_bloom_contains(bloom, "1234567891", 10));
    mu_check(mdv_bloom_contains(bloom, "1234567892", 10));
    mu_check(mdv_bloom_contains(bloom, "1234567893", 10));
    mu_check(!mdv_bloom_contains(bloom, "1234567894", 10));

    mdv_bloom_free(bloom);
}
