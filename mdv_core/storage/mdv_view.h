#pragma once
#include <mdv_table.h>
#include <mdv_rowset.h>


/// Table slice representation
typedef struct mdv_view mdv_view;


typedef mdv_view *   (*mdv_view_retain_fn) (mdv_view *);
typedef uint32_t     (*mdv_view_release_fn)(mdv_view *);
typedef mdv_table *  (*mdv_view_desc_fn)   (mdv_view *);
typedef mdv_rowset * (*mdv_view_fetch_fn)  (mdv_view *, size_t);


/// Interface for view
typedef struct
{
    mdv_view_retain_fn      retain;         ///< Function for view retain
    mdv_view_release_fn     release;        ///< Function for view release
    mdv_view_desc_fn        desc;           ///< Function for table descriptor access
    mdv_view_fetch_fn       fetch;          ///< function for rowset reading
} mdv_iview;


struct mdv_view
{
    mdv_iview const      *vptr;             ///< Interface for view
};


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
