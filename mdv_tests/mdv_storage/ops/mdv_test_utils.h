#pragma once
#include <stddef.h>
#include <stdint.h>
#include <binn.h>
#include <mdv_list.h>


binn * create_test_row(size_t size, uint32_t init);
mdv_list create_test_rows_list(size_t rows, size_t colums);
