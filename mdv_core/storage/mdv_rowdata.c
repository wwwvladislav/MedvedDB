#include "mdv_rowdata.h"
#include "mdv_objects.h"
#include "mdv_storages.h"
#include <mdv_rollbacker.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_serialization.h>
#include <assert.h>


struct mdv_rowdata
{
    mdv_objects     *objects;   ///< DB objects storage
    mdv_uuid        table;      ///< Table identifier
};


mdv_rowdata * mdv_rowdata_open(char const *dir, mdv_uuid const *table)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    mdv_rowdata *rowdata = mdv_alloc(sizeof(mdv_rowdata), "rowdata");

    if (!rowdata)
    {
        MDV_LOGE("No free space of memory for rowdata storage");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, rowdata, "rowdata");

    rowdata->table = *table;

    rowdata->objects = mdv_objects_open(dir, MDV_STRG_UUID(table));

    if (!rowdata->objects)
    {
        MDV_LOGE("Rowdata storage '%s' wasn't created", MDV_STRG_UUID(table));
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_objects_release, rowdata->objects);

    mdv_rollbacker_free(rollbacker);

    return rowdata;
}


mdv_rowdata * mdv_rowdata_retain(mdv_rowdata *rowdata)
{
    if (rowdata)
        mdv_objects_retain(rowdata->objects);
    return rowdata;
}


uint32_t mdv_rowdata_release(mdv_rowdata *rowdata)
{
    if (!rowdata)
        return 0;

    uint32_t rc = mdv_objects_release(rowdata->objects);

    if (!rc)
    {
        mdv_free(rowdata, "rowdata");
    }

    return rc;
}


mdv_errno mdv_rowdata_reserve(mdv_rowdata *rowdata, uint32_t range, uint64_t *id)
{
    return mdv_objects_reserve_ids_range(rowdata->objects, range, id);
}


mdv_errno mdv_rowdata_add_raw(mdv_rowdata *rowdata, mdv_objid const *id, mdv_data const *row)
{
    mdv_data const obj_id =
    {
        .size = sizeof *id,
        .ptr = (void*)id
    };

    return mdv_objects_add(rowdata->objects, &obj_id, row);
}


static mdv_rowset * mdv_rowdata_slice_impl(mdv_enumerator       *enumerator,
                                           mdv_table const      *table,
                                           mdv_bitset const     *fields,
                                           size_t                count,
                                           mdv_objid            *rowid,
                                           mdv_row_filter        filter,
                                           void                 *arg)

{
    mdv_rowset *rowset = 0;

    mdv_table_desc const *desc = mdv_table_description(table);

    if ((rowset = mdv_rowset_create(mdv_bitset_count(fields, true))))
    {
        for(size_t i = 0; i < count;)
        {
            mdv_objects_entry const *entry = mdv_enumerator_current(enumerator);

            binn binn_row;

            mdv_rowlist_entry *row = 0;

            if (binn_load(entry->value.ptr, &binn_row))
            {
                row = mdv_unbinn_row_slice(&binn_row, desc, fields);

                binn_free(&binn_row);

                if(!row)
                {
                    MDV_LOGE("Invalid serialized row");
                    binn_free(&binn_row);
                    break;
                }
            }
            else
            {
                MDV_LOGE("Invalid serialized row");
                break;
            }

            assert(entry->key.size == sizeof(mdv_objid));

            *rowid = *(mdv_objid const *)entry->key.ptr;

            int const fst = filter(arg, &row->data);

            if (fst == 1)
            {
                mdv_rowset_emplace(rowset, row);
                ++i;
            }
            else if (fst == 0)
                mdv_free(row, "rowlist_entry");
            else
            {
                MDV_LOGE("Rowdata filter failed");
                break;
            }

            if (mdv_enumerator_next(enumerator) != MDV_OK)
                break;
        }
    }

    return rowset;
}


mdv_rowset * mdv_rowdata_slice_from_begin(mdv_rowdata           *rowdata,
                                          mdv_table const       *table,
                                          mdv_bitset const      *fields,
                                          size_t                 count,
                                          mdv_objid             *rowid,
                                          mdv_row_filter         filter,
                                          void                  *arg)

{
    mdv_rowset *rowset = 0;

    mdv_enumerator *enumerator = mdv_objects_enumerator(rowdata->objects);

    if (enumerator)
    {
        rowset = mdv_rowdata_slice_impl(enumerator, table, fields, count, rowid, filter, arg);
        mdv_enumerator_release(enumerator);
    }

    return rowset;
}


mdv_rowset * mdv_rowdata_slice(mdv_rowdata          *rowdata,
                               mdv_table const      *table,
                               mdv_bitset const     *fields,
                               size_t                count,
                               mdv_objid            *rowid,
                               mdv_row_filter        filter,
                               void                 *arg)

{
    mdv_rowset *rowset = 0;

    mdv_data const key =
    {
        .size = sizeof *rowid,
        .ptr = rowid
    };

    mdv_enumerator *enumerator = mdv_objects_enumerator_from(rowdata->objects, &key);

    if (enumerator)
    {
        do
        {
            mdv_objects_entry const *entry = mdv_enumerator_current(enumerator);

            assert(entry->key.size == sizeof(mdv_objid));

            mdv_objid const *current_rowid = (mdv_objid const *)entry->key.ptr;

            if (current_rowid->node == rowid->node
                && current_rowid->id == rowid->id)
            {
                if (mdv_enumerator_next(enumerator) != MDV_OK)
                    break;
            }

            rowset = mdv_rowdata_slice_impl(enumerator, table, fields, count, rowid, filter, arg);
        }
        while(0);

        mdv_enumerator_release(enumerator);
    }

    return rowset;
}
