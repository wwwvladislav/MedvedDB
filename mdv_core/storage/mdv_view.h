#pragma once
#include "mdv_predicate.h"
#include "mdv_rowdata.h"
#include "mdv_table.h"
#include <mdv_bitset.h>
#include <mdv_uuid.h>


/// Table slice representation
typedef struct mdv_view mdv_view;


/**
 * @brief Creates new view
 */
mdv_view * mdv_view_create(mdv_rowdata      *source,
                           mdv_table        *table,
                           mdv_bitset       *fields,
                           mdv_predicate    *predicate);


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


/**
 * @brief Returns view table descriptor
 * @details View table descriptor is subset of original table which is extracted using fields mask.
 *
 * @param view [in]     Table slice representation
 *
 * @return table descriptor
 */
mdv_table * mdv_view_desc(mdv_view *view);


/**
 * @brief Rowset reading from the table
 *
 * @param view [in]     Table slice representation
 * @param count [in]    Rows number to be fetched
 *
 * @return Rows set or NULL
 */
mdv_rowset * mdv_view_fetch(mdv_view *view, size_t count);
