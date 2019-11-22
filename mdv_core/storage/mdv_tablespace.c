#include "mdv_tablespace.h"
#include "mdv_trlog.h"
#include "mdv_tables.h"
#include "mdv_rowdata.h"
#include "../mdv_config.h"
#include "../mdv_tracker.h"
#include "../event/mdv_evt_table.h"
#include "../event/mdv_evt_rowdata.h"
#include "../event/mdv_evt_trlog.h"
#include "../event/mdv_evt_types.h"
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
    mdv_tables  *tables;        ///< Tables storage
    mdv_mutex    trlogs_mutex;  ///< Mutex for transaction logs guard
    mdv_hashmap *trlogs;        ///< Transaction logs map (Node UUID -> mdv_trlog)
    mdv_mutex    rowdata_mutex; ///< Mutex for rowdata storages guard
    mdv_hashmap *rowdata;       ///< Rowdata storages map (Table UUID -> mdv_rowdata)
    mdv_uuid     uuid;          ///< Current node UUID
    mdv_ebus    *ebus;          ///< Events bus
};


/// Transaction log storage reference
typedef struct
{
    mdv_uuid   uuid;    ///< Storage UUID
    mdv_trlog *trlog;   ///< Transaction log storage
} mdv_trlog_ref;


/// Rowdata storage reference
typedef struct
{
    mdv_uuid     uuid;      ///< Storage UUID
    mdv_rowdata *rowdata;   ///< Rowdata storage
} mdv_rowdata_ref;


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
 */
static mdv_table * mdv_tablespace_log_create_table(mdv_tablespace *tablespace, mdv_table_desc const *desc);


/**
 * @brief Insert new record into the transaction log for data insertion into the table.
 */
static mdv_errno mdv_tablespace_log_rowset(mdv_tablespace *tablespace, mdv_uuid const *table_id, binn *rows);


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
                .trlog = mdv_trlog_open(MDV_CONFIG.storage.trlog.ptr, uuid)
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


static mdv_rowdata * mdv_tablespace_rowdata_create(mdv_tablespace *tablespace, mdv_uuid const *table_id)
{
    mdv_rowdata_ref *ref = 0;

    if (mdv_mutex_lock(&tablespace->rowdata_mutex) == MDV_OK)
    {
        ref = mdv_hashmap_find(tablespace->rowdata, table_id);

        if(!ref)
        {
            mdv_table *table = mdv_tables_get(tablespace->tables, table_id);

            if (table)
            {
                mdv_rowdata_ref new_ref =
                {
                    .uuid = *table_id,
                    .rowdata = mdv_rowdata_open(MDV_CONFIG.storage.rowdata.ptr, table)
                };

                mdv_table_release(table);

                if (new_ref.rowdata)
                {
                    ref = mdv_hashmap_insert(tablespace->rowdata, &new_ref, sizeof new_ref);

                    if (!ref)
                    {
                        mdv_rowdata_release(new_ref.rowdata);
                        new_ref.rowdata = 0;
                    }
                }
            }
            else
                MDV_LOGE("Table wit UUID '%s' not found", mdv_uuid_to_str(table_id).ptr);
        }

        mdv_mutex_unlock(&tablespace->rowdata_mutex);
    }

    return ref ? mdv_rowdata_retain(ref->rowdata) : 0;
}


static mdv_errno mdv_tablespace_evt_create_table(void *arg, mdv_event *event)
{
    mdv_tablespace       *tablespace   = arg;
    mdv_evt_create_table *create_table = (mdv_evt_create_table *)event;

    mdv_table *table = mdv_tablespace_log_create_table(tablespace, create_table->desc);

    if (table)
    {
        MDV_LOGI("New table '%s' is created", mdv_uuid_to_str(mdv_table_uuid(table)).ptr);
        create_table->table_id = *mdv_table_uuid(table);
        mdv_table_release(table);
        return MDV_OK;
    }

    return MDV_FAILED;
}


static mdv_errno mdv_tablespace_evt_rowdata_insert(void *arg, mdv_event *event)
{
    mdv_tablespace          *tablespace  = arg;
    mdv_evt_rowdata_ins_req *rowdata_ins = (mdv_evt_rowdata_ins_req *)event;
    return mdv_tablespace_log_rowset(tablespace, &rowdata_ins->table_id, rowdata_ins->rows);
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
    { MDV_EVT_CREATE_TABLE,   mdv_tablespace_evt_create_table },
    { MDV_EVT_ROWDATA_INSERT, mdv_tablespace_evt_rowdata_insert },
    { MDV_EVT_TRLOG_APPLY,    mdv_tablespace_evt_trlog_apply },
};


mdv_tablespace * mdv_tablespace_open(mdv_uuid const *uuid, mdv_ebus *ebus)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(7);

    mdv_tablespace *tablespace = mdv_alloc(sizeof(mdv_tablespace), "tablespace");

    if (!tablespace)
    {
        MDV_LOGE("No memory for new tablespace");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, tablespace, "tablespace");

    tablespace->uuid = *uuid;

    tablespace->tables = mdv_tables_open(MDV_CONFIG.storage.path.ptr);

    if (!tablespace->tables)
    {
        MDV_LOGE("Tables storage creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_tables_release, tablespace->tables);

    if (mdv_mutex_create(&tablespace->trlogs_mutex) != MDV_OK)
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

    if (mdv_mutex_create(&tablespace->rowdata_mutex) != MDV_OK)
    {
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &tablespace->rowdata_mutex);

    tablespace->rowdata = mdv_hashmap_create(mdv_rowdata_ref,
                                             uuid,
                                             64,
                                             mdv_uuid_hash,
                                             mdv_uuid_cmp);

    if (!tablespace->rowdata)
    {
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, tablespace->rowdata);

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

        mdv_tables_release(tablespace->tables);

        mdv_hashmap_foreach(tablespace->trlogs, mdv_trlog_ref, ref)
            mdv_trlog_release(ref->trlog);
        mdv_hashmap_release(tablespace->trlogs);
        mdv_mutex_free(&tablespace->trlogs_mutex);

        mdv_hashmap_foreach(tablespace->rowdata, mdv_rowdata_ref, ref)
            mdv_rowdata_release(ref->rowdata);
        mdv_hashmap_release(tablespace->rowdata);
        mdv_mutex_free(&tablespace->rowdata_mutex);

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


static mdv_table * mdv_tablespace_log_create_table(mdv_tablespace *tablespace, mdv_table_desc const *desc)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    mdv_trlog *trlog = mdv_tablespace_trlog_create(tablespace, &tablespace->uuid);

    if (!trlog)
    {
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_trlog_release, trlog);

    mdv_uuid const uuid = mdv_uuid_generate();

    mdv_table *table = mdv_table_create(&uuid, desc);

    if (!table)
    {
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_table_release, table);

    binn obj;

    if (!mdv_binn_table(table, &obj))
    {
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &obj);

    int const binn_obj_size = binn_size(&obj);

    size_t const op_size = offsetof(mdv_trlog_op, payload)
                            + binn_obj_size;

    mdv_trlog_op *op = mdv_staligned_alloc(sizeof(uint64_t), op_size, "trlog_op");

    if (!op)
    {
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_stfree, op, "trlog_op");

    op->size = op_size;
    op->type = MDV_OP_TABLE_CREATE;
    memcpy(op->payload, binn_ptr(&obj), binn_obj_size);

    if (!mdv_trlog_add_op(trlog, op))
    {
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_table_retain(table);

    mdv_rollback(rollbacker);

    mdv_tablespace_trlog_changed_notify(tablespace);

    return table;
}


static mdv_errno mdv_tablespace_log_rowset(mdv_tablespace *tablespace, mdv_uuid const *table_id, binn *rowset)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    mdv_trlog *trlog = mdv_tablespace_trlog_create(tablespace, &tablespace->uuid);

    if (!trlog)
    {
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_trlog_release, trlog);

    binn binn_uuid;

    if (!mdv_binn_uuid(table_id, &binn_uuid))
    {
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &binn_uuid);

    int const binn_uuid_size = binn_size(&binn_uuid);
    int const binn_rowset_size = binn_size(rowset);

    size_t const op_size = offsetof(mdv_trlog_op, payload)
                            + binn_uuid_size
                            + binn_rowset_size;

    mdv_trlog_op *op = mdv_staligned_alloc(sizeof(uint64_t), op_size, "trlog_op");

    if (!op)
    {
        mdv_rollback(rollbacker);
        return MDV_NO_MEM;
    }

    mdv_rollbacker_push(rollbacker, mdv_stfree, op, "trlog_op");

    op->size = op_size;
    op->type = MDV_OP_ROW_INSERT;

    uint8_t *payload = op->payload;

    memcpy(payload, binn_ptr(&binn_uuid), binn_uuid_size);
    payload += binn_uuid_size;
    memcpy(payload, binn_ptr(rowset), binn_rowset_size);
    payload += binn_rowset_size;

    if (!mdv_trlog_add_op(trlog, op))
    {
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollback(rollbacker);

    mdv_tablespace_trlog_changed_notify(tablespace);

    return MDV_OK;
}


typedef struct
{
    mdv_tablespace *tablespace;
    uint32_t        node_id;
} mdv_tablespace_trlog_apply_context;


static bool mdv_tablespace_trlog_apply(void *arg, mdv_trlog_op *op)
{
    mdv_tablespace_trlog_apply_context *context = arg;
    mdv_tablespace *tablespace = context->tablespace;

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
            mdv_uuid table_id;

            ret = mdv_unbinn_uuid(&obj, &table_id);

            if (!ret)
            {
                MDV_LOGE("Rowset insertion failed. Invalid table UUID.");
                break;
            }

            uint8_t *payload = op->payload + binn_size(&obj);

            binn rowset_obj;

            ret = binn_load(op->payload + binn_size(&obj), &rowset_obj);

            if (!ret)
            {
                MDV_LOGE("Invalid rowset");
                break;
            }

            mdv_rowdata *rowdata = mdv_tablespace_rowdata_create(tablespace, &table_id);

            if (rowdata)
            {
                MDV_LOGE("TODO: MDV_OP_ROW_INSERT");
                mdv_rowdata_release(rowdata);
            }

            binn_free(&rowset_obj);

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
        mdv_tablespace_trlog_apply_context context =
        {
            .tablespace = tablespace,
            .node_id = MDV_LOCAL_ID         /// TODO: Convert storage to node_id
        };

        while(mdv_trlog_apply(trlog,
                              MDV_CONFIG.committer.batch_size,
                              &context,
                              mdv_tablespace_trlog_apply)
                >= MDV_CONFIG.committer.batch_size);

        mdv_trlog_release(trlog);
    }

    return true;
}

