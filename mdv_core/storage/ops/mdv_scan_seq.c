#include "mdv_scan_seq.h"
#include <mdv_objid.h>
#include <mdv_rollbacker.h>
#include <mdv_serialization.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <assert.h>
#include <string.h>


typedef struct
{
    mdv_op                  base;
    atomic_uint_fast32_t    ref_counter;
    mdv_table              *table;
    mdv_enumerator         *rows;
    mdv_rowlist_entry      *current;
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
            mdv_free(scanner->current, "rowlist_entry");
            mdv_enumerator_release(scanner->rows);
            mdv_table_release(scanner->table);
            memset(scanner, sizeof *scanner, 0);
            mdv_free(scanner, "scan_seq");
        }
    }

    return rc;
}


static mdv_errno mdv_scan_seq_reset(mdv_op *op)
{
    mdv_scan_seq_t *scanner = (mdv_scan_seq_t *)op;
    mdv_free(scanner->current, "rowlist_entry");
    scanner->current = 0;
    scanner->end = false;
    return mdv_enumerator_reset(scanner->rows);
}


static mdv_row const * mdv_scan_seq_next(mdv_op *op)
{
    mdv_scan_seq_t *scanner = (mdv_scan_seq_t *)op;

    if (scanner->end)
        return 0;

    mdv_table_desc const *desc = mdv_table_description(scanner->table);

    mdv_objects_entry const *entry = mdv_enumerator_current(scanner->rows);
    assert(entry->key.size == sizeof(mdv_objid));
    mdv_objid const *current_rowid = (mdv_objid const *)entry->key.ptr;

    mdv_free(scanner->current, "rowlist_entry");
    scanner->current = 0;

    binn binn_row;

    if (binn_load(entry->value.ptr, &binn_row))
    {
        scanner->current = mdv_unbinn_row(&binn_row, desc);

        binn_free(&binn_row);

        if (!scanner->current)
            MDV_LOGE("Invalid serialized row");
    }
    else
        MDV_LOGE("Invalid serialized row");

    scanner->end = mdv_enumerator_next(scanner->rows) != MDV_OK;

    return scanner->current
            ? &scanner->current->data
            : 0;
}


mdv_op * mdv_scan_seq(mdv_table *table, mdv_objects *objects)
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

    scanner->rows = mdv_objects_enumerator(objects);

    if (!scanner->rows)
    {
        MDV_LOGE("Object enumeratior creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_enumerator_release, scanner->rows);

    mdv_rollbacker_free(rollbacker);

    atomic_init(&scanner->ref_counter, 1);

    scanner->table = mdv_table_retain(table);
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
