#include "mdv_tables.h"
#include "mdv_objects.h"
#include "mdv_storages.h"
#include <mdv_rollbacker.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_serialization.h>
#include <mdv_systbls.h>
#include <assert.h>


struct mdv_tables
{
    mdv_objects *objects;   ///< DB objects storage
    mdv_table   *desc;      ///< Table descriptor
};


static mdv_field const MDV_TABLES_FIELDS[] =
{
    { MDV_FLD_TYPE_BYTE, 16, "ID" },    // UUID
    { MDV_FLD_TYPE_CHAR,  0, "Name" },  // char *
};


static mdv_table_desc const MDV_TABLES_DESC =
{
    .name = "mdv_tables",
    .size = 2,
    .fields = MDV_TABLES_FIELDS
};


mdv_tables * mdv_tables_open(char const *root_dir)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_tables *tables = mdv_alloc(sizeof(mdv_tables), "tables");

    if (!tables)
    {
        MDV_LOGE("No free space of memory for tables storage");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, tables, "tables");

    tables->desc = mdv_table_create(&MDV_SYSTBL_TABLES, &MDV_TABLES_DESC);

    if (!tables->desc)
    {
        MDV_LOGE("No free space of memory for tables storage");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_table_release, tables->desc);

    tables->objects = mdv_objects_open(root_dir, MDV_STRG_TABLES);

    if (!tables->objects)
    {
        MDV_LOGE("Storage '%s' wasn't created", MDV_STRG_TABLES);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_objects_release, tables->objects);

    mdv_rollbacker_free(rollbacker);

    return tables;
}


mdv_tables * mdv_tables_retain(mdv_tables *tables)
{
    if (tables)
        mdv_objects_retain(tables->objects);
    return tables;
}


uint32_t mdv_tables_release(mdv_tables *tables)
{
    if (!tables)
        return 0;

    uint32_t rc = mdv_objects_release(tables->objects);

    if (!rc)
    {
        mdv_table_release(tables->desc);
        mdv_free(tables, "tables");
    }

    return rc;
}


mdv_errno mdv_tables_add_raw(mdv_tables *tables, mdv_uuid const *uuid, mdv_data const *table)
{
    mdv_data const id =
    {
        .size = sizeof *uuid,
        .ptr = (void*)uuid
    };

    return mdv_objects_add(tables->objects, &id, table);
}



static void * mdv_table_restore(mdv_data const *data)
{
    binn obj;

    if(!binn_load(data->ptr, &obj))
    {
        MDV_LOGW("Object reading failed");
        return 0;
    }

    mdv_table *table = mdv_unbinn_table(&obj);

    binn_free(&obj);

    return table;
}


mdv_table * mdv_tables_get(mdv_tables *tables, mdv_uuid const *uuid)
{
    mdv_data const id =
    {
        .size = sizeof *uuid,
        .ptr = (void*)uuid
    };

    return mdv_objects_get(tables->objects, &id, mdv_table_restore);
}


mdv_table * mdv_tables_desc(mdv_tables *tables)
{
    return mdv_table_retain(tables->desc);
}


static mdv_rowset * mdv_tables_slice_impl(mdv_enumerator       *enumerator,
                                          mdv_bitset const     *fields,
                                          size_t                count,
                                          mdv_uuid             *rowid,
                                          mdv_row_filter        filter,
                                          void                 *arg)

{
    mdv_rowset *rowset = 0;

    if ((rowset = mdv_rowset_create(mdv_bitset_count(fields, true))))
    {
        for(size_t i = 0; i < count;)
        {
            mdv_objects_entry const *entry = mdv_enumerator_current(enumerator);

            binn obj;

            mdv_rowlist_entry *row = 0;

            if (binn_load(entry->value.ptr, &obj))
            {
                row = mdv_unbinn_table_as_row_slice(&obj, fields);

                binn_free(&obj);

                if(!row)
                {
                    MDV_LOGE("Invalid serialized table");
                    break;
                }
            }
            else
            {
                MDV_LOGE("Invalid serialized table");
                break;
            }

            assert(entry->key.size == sizeof(mdv_uuid));

            *rowid = *(mdv_uuid const *)entry->key.ptr;

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
                MDV_LOGE("Table filter failed");
                break;
            }

            if (mdv_enumerator_next(enumerator) != MDV_OK)
                break;
        }
    }

    return rowset;
}


mdv_rowset * mdv_tables_slice_from_begin(mdv_tables         *tables,
                                         mdv_bitset const   *fields,
                                         size_t              count,
                                         mdv_uuid           *rowid,
                                         mdv_row_filter      filter,
                                         void               *arg)
{
    mdv_rowset *rowset = 0;

    mdv_enumerator *enumerator = mdv_objects_enumerator(tables->objects);

    if (enumerator)
    {
        rowset = mdv_tables_slice_impl(enumerator, fields, count, rowid, filter, arg);
        mdv_enumerator_release(enumerator);
    }

    return rowset;
}


mdv_rowset * mdv_tables_slice(mdv_tables        *tables,
                              mdv_bitset const  *fields,
                              size_t             count,
                              mdv_uuid          *rowid,
                              mdv_row_filter     filter,
                              void              *arg)
{
    mdv_rowset *rowset = 0;

    mdv_data const key =
    {
        .size = sizeof *rowid,
        .ptr = rowid
    };

    mdv_enumerator *enumerator = mdv_objects_enumerator_from(tables->objects, &key);

    if (enumerator)
    {
        do
        {
            mdv_objects_entry const *entry = mdv_enumerator_current(enumerator);

            assert(entry->key.size == sizeof(mdv_uuid));

            mdv_uuid const *current_rowid = (mdv_uuid const *)entry->key.ptr;

            if (mdv_uuid_cmp(current_rowid, rowid) == 0)
            {
                if (mdv_enumerator_next(enumerator) != MDV_OK)
                    break;
            }

            rowset = mdv_tables_slice_impl(enumerator, fields, count, rowid, filter, arg);
        }
        while(0);

        mdv_enumerator_release(enumerator);
    }

    return rowset;
}
