#include "mdv_tablespace.h"
#include "mdv_trlog.h"
#include "mdv_tables.h"
#include "../mdv_config.h"
#include "../mdv_tracker.h"
#include "../event/mdv_table.h"
#include "../event/mdv_trlog.h"
#include "../event/mdv_types.h"
#include <mdv_types.h>
#include <mdv_serialization.h>
#include <mdv_alloc.h>
#include <mdv_rollbacker.h>
#include <mdv_hashmap.h>
#include <mdv_mutex.h>
#include <stddef.h>


/// DB tables space
struct mdv_tablespace
{
    mdv_mutex    trlogs_mutex;  ///< Mutex for transaction logs guard
    mdv_hashmap *trlogs;        ///< Transaction logs map (Node UUID -> mdv_trlog)
    mdv_tables  *tables;        ///< Tables storage
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
static bool mdv_tablespace_log_create_table(mdv_tablespace *tablespace, mdv_table_base *table);


static mdv_trlog * mdv_tablespace_trlog(mdv_tablespace *tablespace, mdv_uuid const *uuid)
{
    mdv_trlog_ref *ref = 0;

    if (mdv_mutex_lock(&tablespace->trlogs_mutex) == MDV_OK)
    {
        ref = mdv_hashmap_find(tablespace->trlogs, uuid);
        mdv_mutex_unlock(&tablespace->trlogs_mutex);
    }

    if (ref)
        return mdv_trlog_retain(ref->trlog);

    return 0;
}


static mdv_trlog * mdv_tablespace_trlog_create(mdv_tablespace *tablespace, mdv_uuid const *uuid)
{
    mdv_trlog_ref *ref = 0;

    if (mdv_mutex_lock(&tablespace->trlogs_mutex) == MDV_OK)
    {
        ref = mdv_hashmap_find(tablespace->trlogs, uuid);

        if(!ref)
        {
            mdv_trlog_ref new_ref =
            {
                .uuid = *uuid,
                .trlog = mdv_trlog_open(uuid, MDV_CONFIG.storage.path.ptr)
            };

            if (new_ref.trlog)
            {
                ref = mdv_hashmap_insert(tablespace->trlogs, &new_ref, sizeof new_ref);

                if (!ref)
                {
                    mdv_trlog_release(new_ref.trlog);
                    new_ref.trlog = 0;
                }
            }
        }

        mdv_mutex_unlock(&tablespace->trlogs_mutex);
    }

    return ref ? mdv_trlog_retain(ref->trlog) : 0;
}


static mdv_errno mdv_tablespace_evt_create_table(void *arg, mdv_event *event)
{
    mdv_tablespace       *tablespace   = arg;
    mdv_evt_create_table *create_table = (mdv_evt_create_table *)event;

    if (mdv_tablespace_log_create_table(tablespace, create_table->table))
    {
        MDV_LOGI("New table '%s' is created", mdv_uuid_to_str(&create_table->table->id).ptr);
        return MDV_OK;
    }

    return MDV_FAILED;
}

static mdv_errno mdv_tablespace_evt_trlog_apply(void *arg, mdv_event *event)
{
    mdv_tablespace      *tablespace = arg;
    mdv_evt_trlog_apply *apply      = (mdv_evt_trlog_apply *)event;

    return mdv_tablespace_log_apply(tablespace, &apply->uuid)
                ? MDV_OK
                : MDV_FAILED;
}


static const mdv_event_handler_type mdv_tablespace_handlers[] =
{
    { MDV_EVT_CREATE_TABLE, mdv_tablespace_evt_create_table },
    { MDV_EVT_TRLOG_APPLY,  mdv_tablespace_evt_trlog_apply },
};


mdv_tablespace * mdv_tablespace_open(mdv_uuid const *uuid, mdv_ebus *ebus)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(5);

    mdv_tablespace *tablespace = mdv_alloc(sizeof(mdv_tablespace), "tablespace");

    if (!tablespace)
    {
        MDV_LOGE("No memory for new tablespace");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, tablespace, "tablespace");

    tablespace->uuid = *uuid;

    mdv_errno err = mdv_mutex_create(&tablespace->trlogs_mutex);

    if (err != MDV_OK)
    {
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &tablespace->trlogs_mutex);

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

    tablespace->tables = mdv_tables_open(MDV_CONFIG.storage.path.ptr);

    if (!tablespace->tables)
    {
        MDV_LOGE("Tables storage creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_tables_release, tablespace->tables);

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

        mdv_tables_release(tablespace->tables);

        mdv_hashmap_release(tablespace->trlogs);

        mdv_mutex_free(&tablespace->trlogs_mutex);

        mdv_free(tablespace, "tablespace");
    }
}

static void mdv_tablespace_trlog_changed_notify(mdv_tablespace *tablespace)
{
    mdv_evt_trlog_changed *evt = mdv_evt_trlog_changed_create();

    if (evt)
    {
        mdv_ebus_publish(tablespace->ebus, &evt->base, MDV_EVT_UNIQUE);
        mdv_evt_trlog_changed_release(evt);
    }
}


static bool mdv_tablespace_log_create_table(mdv_tablespace *tablespace, mdv_table_base *table)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_trlog *trlog = mdv_tablespace_trlog_create(tablespace, &tablespace->uuid);

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

    if (!mdv_trlog_add_op(trlog, op))
    {
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollback(rollbacker);

    mdv_tablespace_trlog_changed_notify(tablespace);

    return true;
}


static bool mdv_tablespace_trlog_apply(void *arg, mdv_trlog_op *op)
{
    mdv_tablespace *tablespace = arg;

    binn obj;

    if (!binn_load(op->payload, &obj))
    {
        MDV_LOGE("Invalid transaction operation");
        return false;
    }

    bool ret = true;

    switch(op->type)
    {
        case MDV_OP_TABLE_CREATE:
        {
            mdv_uuid const *uuid = mdv_unbinn_table_uuid(&obj);

            if (uuid)
            {
                mdv_data const data =
                {
                    .size = binn_size(&obj),
                    .ptr = binn_ptr(&obj)
                };

                ret = mdv_tables_add_raw(tablespace->tables, uuid, &data) == MDV_OK;
            }
            else
                MDV_LOGE("Table creation failed. Invalid TR log operation.");

            break;
        }

        case MDV_OP_ROW_INSERT:
        {
            MDV_LOGE("TODO: MDV_OP_ROW_INSERT");
            break;
        }

        default:
            MDV_LOGE("Unsupported DB operation");
    }

    binn_free(&obj);

    return ret;
}


bool mdv_tablespace_log_apply(mdv_tablespace *tablespace, mdv_uuid const *storage)
{
    mdv_trlog *trlog = mdv_tablespace_trlog(tablespace, storage);

    if (trlog)
    {
        while(mdv_trlog_apply(trlog,
                              MDV_CONFIG.committer.batch_size,
                              tablespace,
                              mdv_tablespace_trlog_apply)
                >= MDV_CONFIG.committer.batch_size);

        mdv_trlog_release(trlog);
    }

    return true;
}

