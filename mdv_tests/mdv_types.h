#pragma once
#include "mdv_types/mdv_serialization.h"
#include "mdv_types/mdv_rowset.h"
#include "mdv_types/mdv_table.h"
#include "mdv_types/mdv_table_desc.h"


MU_TEST_SUITE(types)
{
    MU_RUN_TEST(types_serialization);
    MU_RUN_TEST(types_rowset);
    MU_RUN_TEST(types_table);
    MU_RUN_TEST(types_table_desc);
}
