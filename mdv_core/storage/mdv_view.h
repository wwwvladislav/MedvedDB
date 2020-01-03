#pragma once
#include "mdv_predicate.h"
#include "mdv_rowdata.h"
#include <mdv_bitset.h>
#include <mdv_uuid.h>


/// Table slice representation
typedef struct mdv_view mdv_view;


/**
 * @brief Creates new view
 */
mdv_view * mdv_view_create(mdv_rowdata  *source,
                           mdv_bitset   *fields,
                           char const   *filter);


/**
 * @brief Retains view.
 * @details Reference counter is increased by one.
 */
mdv_view * mdv_view_retain(mdv_view *view);


/**
 * @brief Releases view.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the view's destructor is called.
 */
uint32_t mdv_view_release(mdv_view *view);

