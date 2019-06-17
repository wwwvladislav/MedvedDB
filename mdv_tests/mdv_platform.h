#pragma once
#include "mdv_platform/mdv_filesystem.h"
#include "mdv_platform/mdv_stack.h"
#include "mdv_platform/mdv_string.h"
#include "mdv_platform/mdv_bloom.h"


MU_TEST_SUITE(platform)
{
    MU_RUN_TEST(platform_mkdir);
    MU_RUN_TEST(platform_stack);
    MU_RUN_TEST(platform_string);
    MU_RUN_TEST(platform_bloom);
}
