#include "mdv_view.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <stdatomic.h>

// TODO: Get compiled expressions from cache


struct mdv_view
{
    atomic_uint_fast32_t  rc;               ///< References counter
    mdv_rowdata          *source;           ///< Rows source
    mdv_bitset           *fields;           ///< Fields mask
    mdv_vector           *filter;           ///< Predicate for rows filtering
};


mdv_view * mdv_view_create(mdv_rowdata  *source,
                           mdv_bitset   *fields,
                           char const   *filter)
{
    mdv_view *view = (mdv_view *)mdv_alloc(sizeof(mdv_view), "view");

    if (!view)
    {
        MDV_LOGE("View creation failed. No memory.");
        return 0;
    }

    view->filter = mdv_predicate_parse(filter);

    if (!view->filter)
    {
        MDV_LOGE("View creation failed.");
        mdv_free(view, "view");
        return 0;
    }

    atomic_init(&view->rc, 1);

    view->source = mdv_rowdata_retain(source);
    view->fields = mdv_bitset_retain(fields);

    return view;
}


static void mdv_view_free(mdv_view *view)
{
    mdv_rowdata_release(view->source);
    mdv_bitset_release(view->fields);
    mdv_vector_release(view->filter);
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
