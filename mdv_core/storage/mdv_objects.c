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


static bool mdv_objects_is_deleted(mdv_objects     *objs,
                                   mdv_map         *map,
                                   mdv_transaction *transaction,
                                   mdv_data const  *key)
{
    mdv_data v = { 0, 0 };
    bool ret = mdv_map_get(map, transaction, key, &v);
    return ret;
}


mdv_errno mdv_objects_add(mdv_objects *objs, mdv_data const *id, mdv_data const *obj)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(objs->storage);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open objects map
    mdv_map objs_map = mdv_map_open(&transaction,
                                    MDV_MAP_OBJECTS,
                                    MDV_MAP_CREATE);

    if (!mdv_map_ok(objs_map))
    {
        MDV_LOGE("Table '%s' not opened", MDV_MAP_OBJECTS);
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &objs_map);

    // Open removed objects table
    mdv_map rem_map = mdv_map_open(&transaction, MDV_MAP_REMOVED, MDV_MAP_CREATE);

    if (!mdv_map_ok(rem_map))
    {
        MDV_LOGE("Table '%s' not opened", MDV_MAP_REMOVED);
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &rem_map);


    if (!mdv_objects_is_deleted(objs,
                                &rem_map,
                                &transaction,
                                id))    // Delete op has priority
    {
        if (!mdv_map_put_unique(&objs_map, &transaction, id, obj))
            MDV_LOGW("Object is already exist.");

        if (!mdv_transaction_commit(&transaction))
        {
            MDV_LOGE("Object insertion failed.");
            mdv_rollback(rollbacker);
            return MDV_FAILED;
        }
    }
    else
        mdv_transaction_abort(&transaction);

    mdv_map_close(&objs_map);
    mdv_map_close(&rem_map);

    mdv_rollbacker_free(rollbacker);

    return MDV_OK;
}

