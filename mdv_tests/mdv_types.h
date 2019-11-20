#pragma once
#include "mdv_types/mdv_serialization.h"
#include "mdv_types/mdv_rowset.h"


MU_TEST_SUITE(types)
{
    MU_RUN_TEST(types_serialization);
    MU_RUN_TEST(types_rowset);
}
