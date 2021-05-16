#include "mdv_2pset.h"
#include "mdv_lmdb.h"
#include "mdv_names.h"
#include <mdv_rollbacker.h>
#include <mdv_alloc.h>
#include <mdv_mutex.h>
#include <mdv_log.h>


static uint16_t MDV_OBJECTS_IDGEN = 0;

static const size_t LMDB_MAP_SIZE = 4294963200u; ///< The maximum size of the LMDB map size.

struct mdv_2pset
{
    mdv_lmdb        *storage;       ///< objects storage
    mdv_mutex        idgen_mutex;   ///< mutex for objects identifiers generator
    uint64_t         idgen;         ///< last free object identifier
};


static bool mdv_objects_idgen_init(mdv_2pset *objs)
{
    objs->idgen = 0;

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

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


mdv_2pset * mdv_2pset_open(char const *root_dir, char const *storage_name)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_2pset *objs = mdv_alloc(sizeof(mdv_2pset));

    if (!objs)
    {
        MDV_LOGE("No free space of memory for objects storage");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, objs);

    objs->storage = mdv_storage_open(root_dir,
                                     storage_name,
                                     MDV_STRG_OBJECTS_MAPS,
                                     MDV_STRG_NOSUBDIR,
                                     LMDB_MAP_SIZE);

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


mdv_2pset * mdv_2pset_retain(mdv_2pset *objs)
{
    if (objs)
        mdv_storage_retain(objs->storage);
    return objs;
}


uint32_t mdv_2pset_release(mdv_2pset *objs)
{
    if (!objs)
        return 0;

    uint32_t rc = mdv_storage_release(objs->storage);

    if (!rc)
    {
        mdv_mutex_free(&objs->idgen_mutex);
        mdv_free(objs);
    }

    return rc;
}


mdv_errno mdv_2pset_reserve_ids_range(mdv_2pset *objs, uint32_t range, uint64_t *id)
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


static bool mdv_objects_is_deleted(mdv_2pset     *objs,
                                   mdv_map         *map,
                                   mdv_transaction *transaction,
                                   mdv_data const  *key)
{
    mdv_data v = { 0, 0 };
    bool ret = mdv_map_get(map, transaction, key, &v);
    return ret;
}


mdv_errno mdv_2pset_add(mdv_2pset *objs, mdv_data const *id, mdv_data const *obj)
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


mdv_errno mdv_2pset_add_batch(mdv_2pset *objs, void *arg, bool (*next)(void *arg, mdv_data *id, mdv_data *obj))
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

    bool commit = false;

    mdv_data id, obj;

    while(next(arg, &id, &obj))
    {
        if (!mdv_objects_is_deleted(objs,
                                    &rem_map,
                                    &transaction,
                                    &id))    // Delete op has priority
        {
            if (mdv_map_put_unique(&objs_map, &transaction, &id, &obj))
                commit = true;
            else
                MDV_LOGW("Object is already exist.");
        }
        else
            mdv_transaction_abort(&transaction);

    }

    if (commit)
    {
        if (!mdv_transaction_commit(&transaction))
        {
            MDV_LOGE("Objects insertion failed.");
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


void * mdv_2pset_get(mdv_2pset *objs, mdv_data const *id, void * (*restore)(mdv_data const *))
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


/// Objects set enumerator
typedef struct
{
    mdv_enumerator          base;           ///< Base type for rowset enumerator
    mdv_2pset              *objects;        ///< Objects storage
    mdv_map                 map;            ///< Objects map
    mdv_transaction         transaction;    ///< Transaction
    mdv_cursor              cursor;         ///< Cursor for objects access
    mdv_kvdata              current;        ///< Current object key and value
} mdv_objects_enumerator_impl;


static mdv_enumerator * mdv_objects_enumerator_impl_retain(mdv_enumerator *enumerator)
{
    atomic_fetch_add_explicit(&enumerator->rc, 1, memory_order_acquire);
    return enumerator;
}


static uint32_t mdv_objects_enumerator_impl_release(mdv_enumerator *enumerator)
{
    uint32_t rc = 0;

    if (enumerator)
    {
        mdv_objects_enumerator_impl *impl = (mdv_objects_enumerator_impl *)enumerator;

        rc = atomic_fetch_sub_explicit(&enumerator->rc, 1, memory_order_release) - 1;

        if (!rc)
        {
            mdv_cursor_close(&impl->cursor);
            mdv_transaction_abort(&impl->transaction);
            mdv_map_close(&impl->map);
            mdv_2pset_release(impl->objects);
            mdv_free(enumerator);
        }
    }

    return rc;
}


static mdv_errno mdv_objects_enumerator_impl_reset(mdv_enumerator *enumerator)
{
    mdv_objects_enumerator_impl *impl = (mdv_objects_enumerator_impl *)enumerator;
    return mdv_cursor_get(&impl->cursor, &impl->current.key, &impl->current.value, MDV_CURSOR_FIRST)
                ? MDV_OK
                : MDV_FAILED;
}


static mdv_errno mdv_objects_enumerator_impl_next(mdv_enumerator *enumerator)
{
    mdv_objects_enumerator_impl *impl = (mdv_objects_enumerator_impl *)enumerator;
    return mdv_cursor_get(&impl->cursor,
                          &impl->current.key,
                          &impl->current.value,
                          MDV_CURSOR_NEXT)
                ? MDV_OK
                : MDV_FAILED;
}


static void * mdv_objects_enumerator_impl_current(mdv_enumerator *enumerator)
{
    mdv_objects_enumerator_impl *impl = (mdv_objects_enumerator_impl *)enumerator;
    return &impl->current;
}


static mdv_enumerator * mdv_objects_enumerator_impl_create(mdv_2pset *objs, mdv_data const *key, mdv_cursor_op op)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_objects_enumerator_impl *enumerator =
            mdv_alloc(sizeof(mdv_objects_enumerator_impl));

    if (!enumerator)
    {
        MDV_LOGE("No memory for objects iterator");
        return 0;
    }

    memset(enumerator, 0, sizeof(mdv_objects_enumerator_impl));

    if (key)
        enumerator->current.key = *key;

    mdv_rollbacker_push(rollbacker, mdv_free, enumerator);

    atomic_init(&enumerator->base.rc, 1);

    static mdv_ienumerator const vtbl =
    {
        .retain = mdv_objects_enumerator_impl_retain,
        .release = mdv_objects_enumerator_impl_release,
        .reset = mdv_objects_enumerator_impl_reset,
        .next = mdv_objects_enumerator_impl_next,
        .current = mdv_objects_enumerator_impl_current
    };

    enumerator->base.vptr = &vtbl;

    // Start transaction
    enumerator->transaction = mdv_transaction_start(objs->storage);

    if (!mdv_transaction_ok(enumerator->transaction))
    {
        MDV_LOGE("Transaction not started");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &enumerator->transaction);

    // Open objects map
    enumerator->map = mdv_map_open(&enumerator->transaction, MDV_MAP_OBJECTS, 0);

    if (!mdv_map_ok(enumerator->map))
    {
        MDV_LOGE("Table '%s' not opened", MDV_MAP_OBJECTS);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &enumerator->map);

    // Create cursor for objects iteration
    enumerator->cursor = mdv_cursor_open_explicit(
                            &enumerator->map,
                            &enumerator->transaction,
                            &enumerator->current.key,
                            &enumerator->current.value,
                            op);

    if (!mdv_cursor_ok(enumerator->cursor))
    {
        MDV_LOGE("Table '%s' cursor not opened", MDV_MAP_OBJECTS);
        mdv_rollback(rollbacker);
        return 0;
    }

    enumerator->objects = mdv_2pset_retain(objs);

    mdv_rollbacker_free(rollbacker);

    return &enumerator->base;
}


mdv_enumerator * mdv_2pset_enumerator(mdv_2pset *objs)
{
    return mdv_objects_enumerator_impl_create(objs, 0, MDV_CURSOR_FIRST);
}


mdv_enumerator * mdv_2pset_enumerator_from(mdv_2pset *objs, mdv_data const *id)
{
    return mdv_objects_enumerator_impl_create(objs, id, MDV_CURSOR_SET_RANGE);
}
