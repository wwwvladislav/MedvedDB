#pragma once
#include "mdv_types.h"
#include <mdv_binn.h>
#include <stdbool.h>


bool             mdv_binn_table(mdv_table_base const *table, binn *obj);
mdv_table_base * mdv_unbinn_table(binn const *obj);
bool             mdv_binn_row(mdv_field const *fields, mdv_row_base const *row, binn *list);
mdv_row_base *   mdv_unbinn_row(binn const *obj, mdv_field const *fields);
