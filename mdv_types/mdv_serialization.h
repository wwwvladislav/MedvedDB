#pragma once
#include "mdv_types.h"
#include <mdv_binn.h>
#include <stdbool.h>


binn *           mdv_binn_table(mdv_table_base const *table);
mdv_table_base * mdv_unbinn_table(binn *obj);
binn *           mdv_binn_row(mdv_field const *fields, mdv_row_base const *row);
mdv_row_base *   mdv_unbinn_row(binn *obj, mdv_field const *fields);
