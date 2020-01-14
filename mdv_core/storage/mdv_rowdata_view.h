#pragma once
#include "mdv_view.h"
#include "mdv_predicate.h"
#include "mdv_rowdata.h"


/**
 * @brief Creates new view
 */
mdv_view * mdv_rowdata_view_create(mdv_rowdata      *source,
                                   mdv_table        *table,
                                   mdv_bitset       *fields,
                                   mdv_predicate    *predicate);
