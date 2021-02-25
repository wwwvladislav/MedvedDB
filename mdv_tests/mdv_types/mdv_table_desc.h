#pragma once
#include <minunit.h>
#include <mdv_table_desc.h>


MU_TEST(types_table_desc)
{
    mdv_table_desc *desc = mdv_table_desc_create("MyTable");
    mu_check(desc);

    mdv_field const fields[] =
    {
        { MDV_FLD_TYPE_CHAR,  0, "Col1" },  // char *
        { MDV_FLD_TYPE_INT32, 2, "Col2" },  // pair { int, int }
        { MDV_FLD_TYPE_BOOL,  1, "Col3" }   // bool
    };

    mu_check(mdv_table_desc_append(desc, fields + 0));
    mu_check(mdv_table_desc_append(desc, fields + 1));
    mu_check(mdv_table_desc_append(desc, fields + 2));

    mdv_table_desc_free(desc);
}
