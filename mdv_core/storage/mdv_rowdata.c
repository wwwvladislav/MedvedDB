#include "mdv_rowdata.h"
#include "mdv_objects.h"
#include "mdv_storages.h"
#include <mdv_rollbacker.h>
#include <mdv_alloc.h>
#include <mdv_log.h>


struct mdv_rowdata
{
    mdv_objects     *objects;   ///< DB objects storage
    mdv_table       *table;     ///< Table descriptor
};


mdv_rowdata * mdv_rowdata_open(char const *dir, mdv_table *table)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_rowdata *rowdata = mdv_alloc(sizeof(mdv_rowdata), "rowdata");

    if (!rowdata)
    {
        MDV_LOGE("No free space of memory for rowdata storage");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, rowdata, "rowdata");

    rowdata->table = mdv_table_retain(table);

    mdv_rollbacker_push(rollbacker, mdv_table_release, rowdata->table);


    rowdata->objects = mdv_objects_open(dir, MDV_STRG_UUID(mdv_table_uuid(table)));

    if (!rowdata->objects)
    {
        MDV_LOGE("Rowdata storage '%s' wasn't created", MDV_STRG_UUID(mdv_table_uuid(table)));
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_objects_release, rowdata->objects);

//    rowdata->table = mdv_table_base_clone(table);
//
//    if (!rowdata->table)
//    {
//        MDV_LOGE("No free space of memory for rowdata storage");
//        mdv_rollback(rollbacker);
//        return 0;
//    }
//
//    mdv_rollbacker_push(rollbacker, mdv_free, rowdata->table, "table");

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
        mdv_table_release(rowdata->table);
        mdv_free(rowdata, "tables");
    }

    return rc;
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
