#include "mdv_objects.h"
#include "mdv_storage.h"
#include "mdv_storages.h"
#include <mdv_rollbacker.h>
#include <mdv_alloc.h>
#include <mdv_log.h>


struct mdv_objects
{
    mdv_storage     *storage;       ///< objects storage
};


mdv_objects * mdv_objects_open(char const *root_dir, char const *storage_name)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    mdv_objects *objs = mdv_alloc(sizeof(mdv_objects), "objects");

    if (!objs)
    {
        MDV_LOGE("No free space of memory for objects storage");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, objs, "objects");

    objs->storage = mdv_storage_open(root_dir,
                                     storage_name,
                                     MDV_STRG_OBJECTS_MAPS,
                                     MDV_STRG_NOSUBDIR);

    if (!objs->storage)
    {
        MDV_LOGE("Storage '%s' wasn't created", storage_name);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_storage_release, objs->storage);

    mdv_rollbacker_free(rollbacker);

    return objs;
}


mdv_objects * mdv_objects_retain(mdv_objects *objs)
{
    if (objs)
        mdv_storage_retain(objs->storage);
    return objs;
}


uint32_t mdv_objects_release(mdv_objects *objs)
{
    if (!objs)
        return 0;

    uint32_t rc = mdv_storage_release(objs->storage);

    if (!rc)
        mdv_free(objs, "objects");

    return rc;
}


mdv_errno mdv_objects_add(mdv_objects *objs, mdv_data const *id, mdv_data const *obj)
{
    // TODO
    return MDV_OK;
}

