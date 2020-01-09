#include "mdv_view.h"
#include "../mdv_config.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_vm.h>
#include <stdatomic.h>

// TODO: Get compiled expressions from cache


struct mdv_view
{
    atomic_uint_fast32_t  rc;               ///< References counter
    mdv_rowdata          *source;           ///< Rows source
    mdv_table            *table;            ///< Table descriptor
    mdv_bitset           *fields;           ///< Fields mask
    mdv_predicate        *filter;           ///< Predicate for rows filtering
    mdv_objid             rowid;            ///< Last read row identifier
    bool                  fetch_from_begin; ///< Flag indicates that data should be fetched from begin
};


mdv_view * mdv_view_create(mdv_rowdata      *source,
                           mdv_table        *table,
                           mdv_bitset       *fields,
                           mdv_predicate    *predicate)
{
    mdv_view *view = (mdv_view *)mdv_alloc(sizeof(mdv_view), "view");

    if (!view)
    {
        MDV_LOGE("View creation failed. No memory.");
        return 0;
    }

    atomic_init(&view->rc, 1);

    view->filter = mdv_predicate_retain(predicate);
    view->source = mdv_rowdata_retain(source);
    view->table  = mdv_table_retain(table);
    view->fields = mdv_bitset_retain(fields);
    view->fetch_from_begin = true;

    return view;
}


static void mdv_view_free(mdv_view *view)
{
    mdv_predicate_release(view->filter);
    mdv_rowdata_release(view->source);
    mdv_table_release(view->table);
    mdv_bitset_release(view->fields);
    mdv_free(view, "view");
}


mdv_view * mdv_view_retain(mdv_view *view)
{
    atomic_fetch_add_explicit(&view->rc, 1, memory_order_acquire);
    return view;
}


uint32_t mdv_view_release(mdv_view *view)
{
    uint32_t rc = 0;

    if (view)
    {
        rc = atomic_fetch_sub_explicit(&view->rc, 1, memory_order_release) - 1;

        if (!rc)
            mdv_view_free(view);
    }

    return rc;
}


static int mdv_view_data_filter(void *arg, mdv_row const *row_slice)
{
    // TODO
    return 1;
}


mdv_rowset * mdv_view_fetch(mdv_view *view, size_t count)
{
    if (view->fetch_from_begin)
    {
        view->fetch_from_begin = false;

        return mdv_rowdata_slice_from_begin(
                    view->source,
                    view->table,
                    view->fields,
                    count,
                    &view->rowid,
                    mdv_view_data_filter,
                    view);
    }

    return mdv_rowdata_slice(
                view->source,
                view->table,
                view->fields,
                count,
                &view->rowid,
                mdv_view_data_filter,
                view);
}
