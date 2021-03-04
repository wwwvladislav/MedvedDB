#include "mdv_project.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <string.h>
#include <stdatomic.h>
#include <binn.h>


typedef struct
{
    mdv_op                  base;
    atomic_uint_fast32_t    ref_counter;
    mdv_op                 *src;
    size_t                  from;
    size_t                  to;
    mdv_kvdata              kvdata;
    binn                   *current;
} mdv_project_range_t;


static mdv_op * mdv_project_range_retain(mdv_op *op)
{
    mdv_project_range_t *projection = (mdv_project_range_t *)op;
    atomic_fetch_add_explicit(&projection->ref_counter, 1, memory_order_acquire);
    return  (mdv_op *)projection;
}


static uint32_t mdv_project_range_release(mdv_op *op)
{
    uint32_t rc = 0;

    if (op)
    {
        mdv_project_range_t *projection = (mdv_project_range_t *)op;

        rc = atomic_fetch_sub_explicit(&projection->ref_counter, 1, memory_order_release) - 1;

        if (!rc)
        {
            binn_free(projection->current);
            mdv_op_release(projection->src);
            memset(projection, sizeof *projection, 0);
            mdv_free(projection, "project_range");
        }
    }

    return rc;
}


static mdv_errno mdv_project_range_reset(mdv_op *op)
{
    mdv_project_range_t *projection = (mdv_project_range_t *)op;
    binn_free(projection->current);
    projection->current = 0;
    return mdv_op_reset(projection->src);
}


static mdv_errno mdv_project_range_next(mdv_op *op, mdv_kvdata *kvdata)
{
    mdv_project_range_t *projection = (mdv_project_range_t *)op;

    binn_free(projection->current);
    projection->current = 0;

    mdv_errno err = mdv_op_next(projection->src, &projection->kvdata);
    if (err != MDV_OK)
        return err;

    binn list;

    if(!binn_load(projection->kvdata.value.ptr, &list))
    {
        MDV_LOGE("Range projection is used only with lists");
        return MDV_FAILED;
    }

    if (binn_type(&list) != BINN_LIST)
    {
        MDV_LOGE("Range projection is used only with lists");
        return MDV_FAILED;
    }

    projection->current = binn_list();

    if (!projection->current)
    {
        MDV_LOGE("No memory for projection list");
        return MDV_NO_MEM;
    }

    binn_iter iter = {};
    binn item = {};
    size_t i = 0;

    binn_list_foreach(&list, item)
    {
        if (i >= projection->from)
        {
            if (!binn_list_add_new(projection->current, &item))
            {
                MDV_LOGE("No memory for projection list item");
                return MDV_NO_MEM;
            }
        }

        if (++i >= projection->to)
            break;
    }

    kvdata->key = projection->kvdata.key;
    kvdata->value.ptr = binn_ptr(projection->current);
    kvdata->value.size = binn_size(projection->current);

    return MDV_OK;
}


mdv_op * mdv_project_range(mdv_op *src, size_t from, size_t to)
{
    mdv_project_range_t *projection = mdv_alloc(sizeof(mdv_project_range_t), "project_range");

    if (!projection)
    {
        MDV_LOGE("No free space of memory for new range projection operation");
        return 0;
    }

    atomic_init(&projection->ref_counter, 1);
    projection->src = mdv_op_retain(src);
    projection->from = from;
    projection->to = to;
    projection->current = 0;

    static mdv_iop const vtbl =
    {
        .retain = mdv_project_range_retain,
        .release = mdv_project_range_release,
        .reset = mdv_project_range_reset,
        .next = mdv_project_range_next
    };

    projection->base.vptr = &vtbl;

    return (mdv_op *)projection;
}
