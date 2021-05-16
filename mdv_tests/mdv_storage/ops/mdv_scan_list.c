#include "mdv_scan_list.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <string.h>
#include <stdatomic.h>


typedef struct
{
    mdv_op                  base;
    atomic_uint_fast32_t    ref_counter;
    mdv_list const         *list;
    mdv_list_entry_base    *current;
    size_t                  idx;
    bool                    end;
} mdv_scan_list_t;


static mdv_op * mdv_scan_list_retain(mdv_op *op)
{
    mdv_scan_list_t *scanner = (mdv_scan_list_t *)op;
    atomic_fetch_add_explicit(&scanner->ref_counter, 1, memory_order_acquire);
    return  (mdv_op *)scanner;
}


static uint32_t mdv_scan_list_release(mdv_op *op)
{
    uint32_t rc = 0;

    if (op)
    {
        mdv_scan_list_t *scanner = (mdv_scan_list_t *)op;

        rc = atomic_fetch_sub_explicit(&scanner->ref_counter, 1, memory_order_release) - 1;

        if (!rc)
        {
            memset(scanner, 0, sizeof *scanner);
            mdv_free(scanner);
        }
    }

    return rc;
}


static mdv_errno mdv_scan_list_reset(mdv_op *op)
{
    mdv_scan_list_t *scanner = (mdv_scan_list_t *)op;
    scanner->current = 0;
    scanner->idx = 0;
    scanner->end = false;
    return MDV_OK;
}


static mdv_errno mdv_scan_list_next(mdv_op *op, mdv_kvdata *kvdata)
{
    mdv_scan_list_t *scanner = (mdv_scan_list_t *)op;

    if (scanner->end)
        return MDV_FALSE;

    if (scanner->current)
    {
        scanner->current = scanner->current->next;
        scanner->idx++;
    }
    else
        scanner->current = scanner->list->next;

    scanner->end = scanner->current == 0;

    if (scanner->current)
    {
        kvdata->key.ptr = &scanner->idx;
        kvdata->key.size = sizeof scanner->idx;
        kvdata->value.ptr = scanner->current->data;
        kvdata->value.size = 0; // unknown size
        return MDV_OK;
    }

    return MDV_FALSE;
}


mdv_op * mdv_scan_list(mdv_list const *list)
{
    mdv_scan_list_t *scanner = mdv_alloc(sizeof(mdv_scan_list_t));

    if (!scanner)
    {
        MDV_LOGE("No free space of memory for new list scanner");
        return 0;
    }

    atomic_init(&scanner->ref_counter, 1);

    scanner->list = list;
    scanner->current = 0;
    scanner->idx = 0;
    scanner->end = false;

    static mdv_iop const vtbl =
    {
        .retain = mdv_scan_list_retain,
        .release = mdv_scan_list_release,
        .reset = mdv_scan_list_reset,
        .next = mdv_scan_list_next
    };

    scanner->base.vptr = &vtbl;

    return (mdv_op *)scanner;
}
