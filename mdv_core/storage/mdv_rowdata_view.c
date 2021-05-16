#include "mdv_rowdata_view.h"
#include "../mdv_config.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_vm.h>
#include <stdatomic.h>


// TODO: Get compiled expressions from cache


typedef struct
{
    mdv_view              base;             ///< Base type for view
    atomic_uint_fast32_t  rc;               ///< References counter
    mdv_rowdata          *source;           ///< Rows source
    mdv_table            *table;            ///< Table descriptor
    mdv_table            *table_slice;      ///< Table descriptor slice
    mdv_bitset           *fields;           ///< Fields mask
    mdv_predicate        *filter;           ///< Predicate for rows filtering
    mdv_objid             rowid;            ///< Last read row identifier
    bool                  fetch_from_begin; ///< Flag indicates that data should be fetched from begin
} mdv_rowdata_view;


static void mdv_rowdata_view_free(mdv_rowdata_view *view)
{
    mdv_predicate_release(view->filter);
    mdv_rowdata_release(view->source);
    mdv_table_release(view->table);
    mdv_table_release(view->table_slice);
    mdv_bitset_release(view->fields);
    mdv_free(view);
}


static mdv_view * mdv_rowdata_view_retain(mdv_view *base)
{
    mdv_rowdata_view *view = (mdv_rowdata_view *)base;
    atomic_fetch_add_explicit(&view->rc, 1, memory_order_acquire);
    return base;
}


static uint32_t mdv_rowdata_view_release(mdv_view *base)
{
    mdv_rowdata_view *view = (mdv_rowdata_view *)base;

    uint32_t rc = 0;

    if (view)
    {
        rc = atomic_fetch_sub_explicit(&view->rc, 1, memory_order_release) - 1;

        if (!rc)
            mdv_rowdata_view_free(view);
    }

    return rc;
}


static mdv_table * mdv_rowdata_view_desc(mdv_view *base)
{
    mdv_rowdata_view *view = (mdv_rowdata_view *)base;
    return mdv_table_retain(view->table_slice);
}


static int mdv_rowdata_view_filter(void *arg, mdv_row const *row_slice)
{
    // TODO
    return 1;
}


static mdv_rowset * mdv_rowdata_view_fetch(mdv_view *base, size_t count)
{
    mdv_rowdata_view *view = (mdv_rowdata_view *)base;

    if (view->fetch_from_begin)
    {
        view->fetch_from_begin = false;

        return mdv_rowdata_slice_from_begin(
                    view->source,
                    view->table,
                    view->fields,
                    count,
                    &view->rowid,
                    mdv_rowdata_view_filter,
                    view);
    }

    return mdv_rowdata_slice(
                view->source,
                view->table,
                view->fields,
                count,
                &view->rowid,
                mdv_rowdata_view_filter,
                view);
}


mdv_view * mdv_rowdata_view_create(mdv_rowdata      *source,
                                   mdv_table        *table,
                                   mdv_bitset       *fields,
                                   mdv_predicate    *predicate)
{
    mdv_rowdata_view *view = (mdv_rowdata_view *)mdv_alloc(sizeof(mdv_rowdata_view));

    if (!view)
    {
        MDV_LOGE("View creation failed. No memory.");
        return 0;
    }

    static mdv_iview const vtbl =
    {
        .retain  = mdv_rowdata_view_retain,
        .release = mdv_rowdata_view_release,
        .desc    = mdv_rowdata_view_desc,
        .fetch   = mdv_rowdata_view_fetch,
    };

    view->table_slice = mdv_table_slice(table, fields);

    if (!view->table_slice)
    {
        MDV_LOGE("View creation failed.");
        mdv_free(view);
        return 0;
    }

    atomic_init(&view->rc, 1);

    view->base.vptr = &vtbl;

    view->filter = mdv_predicate_retain(predicate);
    view->source = mdv_rowdata_retain(source);
    view->table  = mdv_table_retain(table);
    view->fields = mdv_bitset_retain(fields);
    view->fetch_from_begin = true;

    return &view->base;
}
