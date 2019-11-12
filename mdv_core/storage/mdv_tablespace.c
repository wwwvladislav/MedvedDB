#include "mdv_tablespace.h"
#include "../mdv_config.h"
#include "../mdv_tracker.h"
#include "../event/mdv_table.h"
#include "../event/mdv_trlog.h"
#include "../event/mdv_types.h"
#include <mdv_serialization.h>
#include <mdv_alloc.h>
#include <mdv_rollbacker.h>
#include <mdv_hashmap.h>
#include <mdv_mutex.h>
#include <stddef.h>


/// DB tables space
struct mdv_tablespace
{
    mdv_mutex    mutex;         ///< Mutex for storages guard
    mdv_hashmap *trlogs;        ///< Transaction logs map (Node UUID -> mdv_trlog)
    mdv_uuid     uuid;          ///< Current node UUID
    mdv_ebus    *ebus;          ///< Events bus
};


/// Transaction log storage reference
typedef struct
{
    mdv_uuid   uuid;    ///< Storage UUID
    mdv_trlog *trlog;   ///< Transaction log storage
} mdv_trlog_ref;


/// DB operations list
enum
{
    MDV_OP_TABLE_CREATE = 0,    ///< Create table
    MDV_OP_TABLE_DROP,          ///< Drop table
    MDV_OP_ROW_INSERT           ///< Insert data into a table
};


/**
 * @brief Insert new record into the transaction log for new table creation.
 * @details After the successfully table creation new generated table UUID is saved to table->id.
 *
 * @param tablespace [in]   Pointer to a tablespace structure
 * @param table [in] [out]  Table description
 *
 * @return non zero table identifier pointer if operation successfully completed.
 */
static bool mdv_tablespace_create_table(mdv_tablespace *tablespace, mdv_table_base *table);


static mdv_trlog * mdv_tablespace_trlog(mdv_tablespace *tablespace, mdv_uuid const *uuid)
{
    mdv_trlog_ref *ref = 0;

    if (mdv_mutex_lock(&tablespace->mutex) == MDV_OK)
    {
        ref = mdv_hashmap_find(tablespace->trlogs, uuid);
        mdv_mutex_unlock(&tablespace->mutex);
    }

    if (ref)
        return mdv_trlog_retain(ref->trlog);

    mdv_trlog_ref new_ref =
    {
        .uuid = *uuid,
        .trlog = mdv_trlog_open(uuid, MDV_CONFIG.storage.path.ptr)
    };

    if (!new_ref.trlog)
        return 0;

    if (mdv_mutex_lock(&tablespace->mutex) == MDV_OK)
    {
        if (!mdv_hashmap_insert(tablespace->trlogs, &new_ref, sizeof new_ref))
        {
            mdv_trlog_release(new_ref.trlog);
            new_ref.trlog = 0;
        }
        mdv_mutex_unlock(&tablespace->mutex);
    }
    else
    {
        mdv_trlog_release(new_ref.trlog);
        new_ref.trlog = 0;
    }

    return mdv_trlog_retain(new_ref.trlog);
}


static mdv_errno mdv_tablespace_evt_create_table(void *arg, mdv_event *event)
{
    mdv_tablespace       *tablespace   = arg;
    mdv_evt_create_table *create_table = (mdv_evt_create_table *)event;

    if (mdv_tablespace_create_table(tablespace, create_table->table))
    {
        MDV_LOGI("New table '%s' is created", mdv_uuid_to_str(&create_table->table->id).ptr);
        return MDV_OK;
    }

    return MDV_FAILED;
}


static const mdv_event_handler_type mdv_tablespace_handlers[] =
{
    { MDV_EVT_CREATE_TABLE, mdv_tablespace_evt_create_table },
};


mdv_tablespace * mdv_tablespace_open(mdv_uuid const *uuid, mdv_ebus *ebus)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    mdv_tablespace *tablespace = mdv_alloc(sizeof(mdv_tablespace), "tablespace");

    if (!tablespace)
    {
        MDV_LOGE("No memory for new tablespace");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, tablespace, "tablespace");

    tablespace->uuid = *uuid;

    mdv_errno err = mdv_mutex_create(&tablespace->mutex);

    if (err != MDV_OK)
    {
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &tablespace->mutex);

    tablespace->trlogs = mdv_hashmap_create(mdv_trlog_ref,
                                            uuid,
                                            64,
                                            mdv_uuid_hash,
                                            mdv_uuid_cmp);

    if (!tablespace->trlogs)
    {
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, tablespace->trlogs);

    tablespace->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, tablespace->ebus);

    if (mdv_ebus_subscribe_all(tablespace->ebus,
                               tablespace,
                               mdv_tablespace_handlers,
                               sizeof mdv_tablespace_handlers / sizeof *mdv_tablespace_handlers) != MDV_OK)
    {
        MDV_LOGE("Ebus subscription failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_free(rollbacker);

    return tablespace;
}


void mdv_tablespace_close(mdv_tablespace *tablespace)
{
    if (tablespace)
    {
        mdv_ebus_unsubscribe_all(tablespace->ebus,
                                 tablespace,
                                 mdv_tablespace_handlers,
                                 sizeof mdv_tablespace_handlers / sizeof *mdv_tablespace_handlers);

        mdv_ebus_release(tablespace->ebus);

        mdv_hashmap_foreach(tablespace->trlogs, mdv_trlog_ref, ref)
        {
            mdv_trlog_release(ref->trlog);
        }

        mdv_hashmap_release(tablespace->trlogs);

        mdv_mutex_free(&tablespace->mutex);

        mdv_free(tablespace, "tablespace");
    }
}

static void mdv_tablespace_trlog_changed_notify(mdv_tablespace *tablespace)
{
    mdv_evt_trlog_changed *evt = mdv_evt_trlog_changed_create();

    if (evt)
    {
        mdv_ebus_publish(tablespace->ebus, evt, MDV_EVT_UNIQUE);
        mdv_evt_trlog_changed_release(evt);
    }
}


static bool mdv_tablespace_create_table(mdv_tablespace *tablespace, mdv_table_base *table)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_trlog *trlog = mdv_tablespace_trlog(tablespace, &tablespace->uuid);

    if (!trlog)
    {
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_trlog_release, trlog);

    table->id = mdv_uuid_generate();

    binn obj;

    if (!mdv_binn_table(table, &obj))
    {
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &obj);

    int const binn_obj_size = binn_size(&obj);

    size_t const op_size = offsetof(mdv_trlog_op, payload)
                            + binn_obj_size;

    mdv_trlog_op *op = mdv_staligned_alloc(sizeof(uint64_t), op_size, "trlog_op");

    if (!op)
    {
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, mdv_stfree, op, "trlog_op");

    op->size = offsetof(mdv_trlog_op, payload) + binn_obj_size;
    op->type = MDV_OP_TABLE_CREATE;
    memcpy(op->payload, binn_ptr(&obj), binn_obj_size);

    mdv_uuid const *objid = 0;

    if (!mdv_trlog_new_op(trlog, op))
    {
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollback(rollbacker);

    mdv_tablespace_trlog_changed_notify(tablespace);

    return true;
}


mdv_vector * mdv_tablespace_trlogs(mdv_tablespace *tablespace)
{
    mdv_vector *uuids = mdv_vector_create(54, sizeof(mdv_uuid), &mdv_default_allocator);

    if (!uuids)
        return 0;

    if (mdv_mutex_lock(&tablespace->mutex) == MDV_OK)
    {
        mdv_hashmap_foreach(tablespace->trlogs, mdv_trlog_ref, ref)
        {
            mdv_vector_push_back(uuids, &ref->uuid);
        }
        mdv_mutex_unlock(&tablespace->mutex);
    }

    return uuids;
}

/*
static bool mdv_tablespace_log_apply_fn(void *arg, mdv_cfstorage_op const *op)
{
    mdv_tablespace *tablespace = arg;

    mdv_op *db_op = op->op.ptr;

    binn obj;

    if (!binn_load(db_op->payload, &obj))
    {
        MDV_LOGE("Invalid transaction operation");
        return false;
    }

    switch(db_op->op)
    {
        case MDV_OP_TABLE_CREATE:
        {
            MDV_LOGE("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
            break;
        }

        case MDV_OP_ROW_INSERT:
        {
        }

        default:
            MDV_LOGE("Unsupported DB operation");
    }

    binn_free(&obj);

    return true;
}
*/

bool mdv_tablespace_log_apply(mdv_tablespace *tablespace, mdv_uuid const *storage, uint32_t peer_id)
{
//    mdv_cfstorage *cfstorage = mdv_tablespace_cfstorage(tablespace, storage);

//    if (!cfstorage)
//        return false;

//    return mdv_cfstorage_log_apply(cfstorage, peer_id, tablespace, &mdv_tablespace_log_apply_fn);

    return false;
}

