#pragma once
#include "mdv_storage/mdv_predicate.h"
#include "mdv_storage/mdv_paginator.h"
#include "mdv_storage/ops/mdv_scan_seq.h"
#include "mdv_storage/ops/mdv_project.h"
#include "mdv_storage/ops/mdv_select.h"


MU_TEST_SUITE(storage)
{
    MU_RUN_TEST(storage_predicate);
    MU_RUN_TEST(storage_paginator);
    MU_RUN_TEST(op_scan_seq);
    MU_RUN_TEST(op_project_range);
    MU_RUN_TEST(op_project_by_indices);
    MU_RUN_TEST(op_select);
}
