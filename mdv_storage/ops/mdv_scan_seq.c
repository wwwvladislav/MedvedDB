#include "mdv_scan_seq.h"
#include <mdv_rollbacker.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <string.h>
#include <stdatomic.h>


typedef struct
{
    mdv_op                  base;
    atomic_uint_fast32_t    ref_counter;
    mdv_enumerator         *enumerator;
    mdv_kvdata             *current;
    bool                    end;
} mdv_scan_seq_t;


static mdv_op * mdv_scan_seq_retain(mdv_op *op)
{
    mdv_scan_seq_t *scanner = (mdv_scan_seq_t *)op;
    atomic_fetch_add_explicit(&scanner->ref_counter, 1, memory_order_acquire);
    return  (mdv_op *)scanner;
}


static uint32_t mdv_scan_seq_release(mdv_op *op)
{
    uint32_t rc = 0;

    if (op)
    {
        mdv_scan_seq_t *scanner = (mdv_scan_seq_t *)op;

        rc = atomic_fetch_sub_explicit(&scanner->ref_counter, 1, memory_order_release) - 1;

        if (!rc)
        {
            mdv_enumerator_release(scanner->enumerator);
            memset(scanner, 0, sizeof *scanner);
            mdv_free(scanner, "scan_seq");
        }
    }

    return rc;
}


static mdv_errno mdv_scan_seq_reset(mdv_op *op)
{
    mdv_scan_seq_t *scanner = (mdv_scan_seq_t *)op;
    scanner->current = 0;
    scanner->end = false;
    return mdv_enumerator_reset(scanner->enumerator);
}


static mdv_errno mdv_scan_seq_next(mdv_op *op, mdv_kvdata *kvdata)
{
    mdv_scan_seq_t *scanner = (mdv_scan_seq_t *)op;

    if (scanner->end)
        return MDV_FALSE;

    if (scanner->current)
        scanner->end = mdv_enumerator_next(scanner->enumerator) != MDV_OK;

    scanner->current = scanner->end ? 0 : mdv_enumerator_current(scanner->enumerator);

    if (scanner->current)
    {
        *kvdata = *scanner->current;
        return MDV_OK;
    }

    return MDV_FALSE;
}


mdv_op * mdv_scan_seq(mdv_2pset *objects)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    mdv_scan_seq_t *scanner = mdv_alloc(sizeof(mdv_scan_seq_t), "scan_seq");

    if (!scanner)
    {
        MDV_LOGE("No free space of memory for new objects scanner");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, scanner, "scan_seq");

    scanner->enumerator = mdv_2pset_enumerator(objects);

    if (!scanner->enumerator)
    {
        MDV_LOGE("Object enumeratior creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_enumerator_release, scanner->enumerator);

    mdv_rollbacker_free(rollbacker);

    atomic_init(&scanner->ref_counter, 1);

    scanner->current = 0;
    scanner->end = false;

    static mdv_iop const vtbl =
    {
        .retain = mdv_scan_seq_retain,
        .release = mdv_scan_seq_release,
        .reset = mdv_scan_seq_reset,
        .next = mdv_scan_seq_next
    };

    scanner->base.vptr = &vtbl;

    return (mdv_op *)scanner;
}
