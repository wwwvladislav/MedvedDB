#include "mdv_test_utils.h"


binn * create_test_row(size_t size, uint32_t init)
{
    binn *row = binn_list();
    uint32_t n = init;
    for (size_t i = 0; i < size; ++i)
        binn_list_add_uint32(row, n++);
    return row;
}


mdv_list create_test_rows_list(size_t rows, size_t colums)
{
    mdv_list list = {};

    for (size_t i = 0; i < rows; ++i)
    {
        binn *row = create_test_row(colums, 0);
        mdv_list_push_back_data(&list, binn_ptr(row), binn_size(row));
        binn_free(row);
    }

    return list;
}
