#include "mdv_tables.h"
#include "mdv_objects.h"
#include "mdv_storages.h"
#include <mdv_rollbacker.h>
#include <mdv_alloc.h>
#include <mdv_log.h>


struct mdv_tables
{
    mdv_objects *objects;   ///< DB objects storage
};


mdv_tables * mdv_tables_open(char const *root_dir)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    mdv_tables *tables = mdv_alloc(sizeof(mdv_tables), "tables");

    if (!tables)
    {
        MDV_LOGE("No free space of memory for tables storage");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, tables, "tables");

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
        mdv_free(tables, "tables");

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
