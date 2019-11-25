#include "mdv_objects.h"
#include "mdv_storage.h"
#include "mdv_storages.h"
#include <mdv_rollbacker.h>
#include <mdv_alloc.h>
#include <mdv_mutex.h>
#include <mdv_log.h>


static uint8_t MDV_OBJECTS_IDGEN = 0;


struct mdv_objects
{
    mdv_storage     *storage;       ///< objects storage
    mdv_mutex        idgen_mutex;   ///< mutex for objects identifiers generator
    uint64_t         idgen;         ///< last free object identifier
};


static bool mdv_objects_idgen_init(mdv_objects *objs)
{
    objs->idgen = 0;

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(objs->storage);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open objects map
    mdv_map objs_map = mdv_map_open(&transaction,
                                    MDV_MAP_IDGEN,
                                    MDV_MAP_INTEGERKEY | MDV_MAP_SILENT);

    if (!mdv_map_ok(objs_map))
    {
        mdv_rollback(rollbacker);
        return true;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &objs_map);

    mdv_data key = { sizeof MDV_OBJECTS_IDGEN, &MDV_OBJECTS_IDGEN };
    mdv_data value = {};

    if (!mdv_map_get(&objs_map, &transaction, &key, &value))
    {
        mdv_rollback(rollbacker);
        return true;
    }

    objs->idgen = *(uint64_t*)value.ptr;

    mdv_transaction_abort(&transaction);
    mdv_map_close(&objs_map);
    mdv_rollbacker_free(rollbacker);

    return true;
}


mdv_objects * mdv_objects_open(char const *root_dir, char const *storage_name)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

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

    if (mdv_mutex_create(&objs->idgen_mutex) != MDV_OK)
    {
        MDV_LOGE("Mutex for objects identifiers generator not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &objs->idgen_mutex);

    if (!mdv_objects_idgen_init(objs))
    {
        MDV_LOGE("Objects identifiers generator not initialized");
        mdv_rollback(rollbacker);
        return 0;
    }

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
    {
        mdv_mutex_free(&objs->idgen_mutex);
        mdv_free(objs, "objects");
    }

    return rc;
}


mdv_errno mdv_objects_reserve_ids_range(mdv_objects *objs, uint32_t range, uint64_t *id)
{
    mdv_errno err = mdv_mutex_lock(&objs->idgen_mutex);

    if (err != MDV_OK)
    {
        MDV_LOGE("Mutex lock failed");
        return err;
    }

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_rollbacker_push(rollbacker, mdv_mutex_unlock, &objs->idgen_mutex);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(objs->storage);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open idgen map
    mdv_map idgen_map = mdv_map_open(&transaction,
                                     MDV_MAP_IDGEN,
                                     MDV_MAP_INTEGERKEY | MDV_MAP_CREATE);

    if (!mdv_map_ok(idgen_map))
    {
        MDV_LOGE("Table '%s' not opened", MDV_MAP_IDGEN);
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &idgen_map);

    uint64_t new_id = objs->idgen + range;

    mdv_data key = { sizeof MDV_OBJECTS_IDGEN, &MDV_OBJECTS_IDGEN };
    mdv_data value = { sizeof new_id, &new_id };

    if (!mdv_map_put(&idgen_map, &transaction, &key, &value))
    {
        MDV_LOGE("Identifiers range reservation failed");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    if (!mdv_transaction_commit(&transaction))
    {
        MDV_LOGE("Identifiers range reservation failed");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    *id = objs->idgen;

    objs->idgen = new_id;

    mdv_map_close(&idgen_map);

    mdv_rollbacker_free(rollbacker);

    mdv_mutex_unlock(&objs->idgen_mutex);

    return err;
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


void * mdv_objects_get(mdv_objects *objs, mdv_data const *id, void * (*restore)(mdv_data const *))
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(objs->storage);

    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("CFstorage transaction not started");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open objects map
    mdv_map objs_map = mdv_map_open(&transaction,
                                    MDV_MAP_OBJECTS,
                                    MDV_MAP_SILENT);

    if (!mdv_map_ok(objs_map))
    {
        MDV_LOGE("Table '%s' not opened", MDV_MAP_OBJECTS);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &objs_map);

    // Open removed objects table
    mdv_map rem_map = mdv_map_open(&transaction, MDV_MAP_REMOVED, MDV_MAP_SILENT);

    if (!mdv_map_ok(rem_map))
    {
        MDV_LOGE("Table '%s' not opened", MDV_MAP_REMOVED);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &rem_map);

    if (mdv_objects_is_deleted(objs,
                                &rem_map,
                                &transaction,
                                id))
    {
        MDV_LOGE("Object was deleted");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_data value = {};

    if (!mdv_map_get(&objs_map, &transaction, id, &value))
    {
        MDV_LOGW("Object wasn't found.");
        mdv_rollback(rollbacker);
        return 0;
    }

    void *obj = restore(&value);

    mdv_transaction_abort(&transaction);
    mdv_map_close(&objs_map);
    mdv_map_close(&rem_map);

    mdv_rollbacker_free(rollbacker);

    return obj;
}
