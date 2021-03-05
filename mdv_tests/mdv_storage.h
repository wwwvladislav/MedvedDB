#pragma once
#include "mdv_storage/ops/mdv_scan_seq.h"
#include "mdv_storage/ops/mdv_project.h"


MU_TEST_SUITE(storage)
{
    MU_RUN_TEST(op_scan_seq);
    MU_RUN_TEST(op_project_range);
    MU_RUN_TEST(op_project_by_indices);
}
