#include "mdv_select.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <string.h>
#include <stdatomic.h>


typedef struct
{
    mdv_op                  base;
    atomic_uint_fast32_t    ref_counter;
    mdv_op                 *src;
    mdv_predicate          *predicate;
} mdv_select_t;


static mdv_op * mdv_select_retain(mdv_op *op)
{
    mdv_select_t *select = (mdv_select_t *)op;
    atomic_fetch_add_explicit(&select->ref_counter, 1, memory_order_acquire);
    return  (mdv_op *)select;
}


static uint32_t mdv_select_release(mdv_op *op)
{
    uint32_t rc = 0;

    if (op)
    {
        mdv_select_t *select = (mdv_select_t *)op;

        rc = atomic_fetch_sub_explicit(&select->ref_counter, 1, memory_order_release) - 1;

        if (!rc)
        {
            mdv_predicate_release(select->predicate);
            mdv_op_release(select->src);
            memset(select, sizeof *select, 0);
            mdv_free(select, "select");
        }
    }

    return rc;
}


static mdv_errno mdv_select_reset(mdv_op *op)
{
    mdv_select_t *select = (mdv_select_t *)op;
    return mdv_op_reset(select->src);
}


static mdv_errno mdv_select_next(mdv_op *op, mdv_kvdata *kvdata)
{
    mdv_select_t *select = (mdv_select_t *)op;

    mdv_kvdata current;

    mdv_errno err = mdv_op_next(select->src, &current);
    if (err != MDV_OK)
        return err;

    // TODO: use predicate for DB entries selection

    *kvdata = current;

    return MDV_OK;
}


mdv_op * mdv_select(mdv_op *src, mdv_predicate *predicate)
{
    mdv_select_t *select = mdv_alloc(sizeof(mdv_select_t), "select");

    if (!select)
    {
        MDV_LOGE("No free space of memory for new select operation");
        return 0;
    }

    atomic_init(&select->ref_counter, 1);
    select->src = mdv_op_retain(src);
    select->predicate = mdv_predicate_retain(predicate);

    static mdv_iop const vtbl =
    {
        .retain = mdv_select_retain,
        .release = mdv_select_release,
        .reset = mdv_select_reset,
        .next = mdv_select_next
    };

    select->base.vptr = &vtbl;

    return (mdv_op *)select;
}
