#include "mdv_tablespace.h"
#include "mdv_trlog.h"
#include "mdv_tables.h"
#include "mdv_rowdata.h"
#include "../mdv_config.h"
#include "../event/mdv_evt_table.h"
#include "../event/mdv_evt_rowdata.h"
#include "../event/mdv_evt_topology.h"
#include "../event/mdv_evt_trlog.h"
#include "../event/mdv_evt_types.h"
#include <mdv_types.h>
#include <mdv_serialization.h>
#include <mdv_alloc.h>
#include <mdv_rollbacker.h>
#include <mdv_hashmap.h>
#include <mdv_mutex.h>
#include <mdv_safeptr.h>


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
    mdv_safeptr *storage_ids;   ///< Storage identifiers map (hashmap<mdv_storage_id>)
};


/// Stroage identifiers map
typedef struct
{
    mdv_uuid    uuid;   ///< Global unique storage identifier
    uint32_t    id;     ///< local unique storage identifier
} mdv_storage_id;


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
 * @brief Applies a transaction log to the data storage.
 */
bool mdv_tablespace_log_apply(mdv_tablespace *tablespace, mdv_uuid const *storage);


/**
 * @brief Insert new record into the transaction log for new table creation.
 * @details After the successfully table creation new generated table UUID is saved to table->id.
 */
static mdv_table * mdv_tablespace_log_create_table(mdv_tablespace *tablespace, mdv_table_desc const *desc);


/**
 * @brief Insert new record into the transaction log for data insertion into the table.
 */
static mdv_errno mdv_tablespace_log_rowset(mdv_tablespace *tablespace, mdv_uuid const *table_id, binn *rows);


static mdv_hashmap/*<mdv_storage_id>*/ * mdv_tablespace_storage_ids(mdv_topology *topology)
{
    mdv_vector *nodes = mdv_topology_nodes(topology);

    mdv_hashmap *idmap = mdv_hashmap_create(
                            mdv_storage_id,
                            uuid,
                            mdv_vector_size(nodes) * 5 / 4,
                            mdv_uuid_hash,
                            mdv_uuid_cmp);

    if (!idmap)
    {
        MDV_LOGE("No memory for identifiers map");
        mdv_vector_release(nodes);
        return 0;
    }

    mdv_vector_foreach(nodes, mdv_toponode, node)
    {
        mdv_storage_id const storage_id =
        {
            .uuid = node->uuid,
            .id = node->id
        };

        if (!mdv_hashmap_insert(idmap, &storage_id, sizeof storage_id))
        {
            MDV_LOGE("No memory for identifiers map");
            mdv_vector_release(nodes);
            mdv_hashmap_release(idmap);
            return 0;
        }
    }

    mdv_vector_release(nodes);

    return idmap;
}


static bool mdv_tablespace_storage_id(mdv_tablespace *tablespace, mdv_uuid const *uuid, uint32_t *id)
{
    bool res = false;

    mdv_hashmap *idmap = mdv_safeptr_get(tablespace->storage_ids);

    mdv_storage_id *storage_id = mdv_hashmap_find(idmap, uuid);

    if (storage_id)
    {
        *id = storage_id->id;
        res = true;
    }
    else
        MDV_LOGE("Storage idntifier %s not found", mdv_uuid_to_str(uuid).ptr);

    mdv_hashmap_release(idmap);

    return res;
}


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
            uint32_t id;

            if (mdv_tablespace_storage_id(tablespace, uuid, &id))
            {
                mdv_trlog_ref new_ref =
                {
                    .uuid = *uuid,
                    .trlog = mdv_trlog_open(tablespace->ebus, MDV_CONFIG.storage.trlog.ptr, uuid, id)
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
            mdv_rowdata_ref new_ref =
            {
                .uuid = *table_id,
                .rowdata = mdv_rowdata_open(MDV_CONFIG.storage.rowdata.ptr, table_id)
            };

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

        mdv_mutex_unlock(&tablespace->rowdata_mutex);
    }

    return ref ? mdv_rowdata_retain(ref->rowdata) : 0;
}


static mdv_errno mdv_tablespace_evt_get_table(void *arg, mdv_event *event)
{
    mdv_tablespace  *tablespace = arg;
    mdv_evt_table   *get_table  = (mdv_evt_table *)event;

    get_table->table = mdv_tables_get(tablespace->tables, &get_table->table_id);

    return get_table->table ? MDV_OK : MDV_FAILED;
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


static mdv_errno mdv_tablespace_evt_trlog_get(void *arg, mdv_event *event)
{
    mdv_tablespace *tablespace = arg;
    mdv_evt_trlog  *get = (mdv_evt_trlog *)event;
    get->trlog = get->create
                    ? mdv_tablespace_trlog_create(tablespace, &get->uuid)
                    : mdv_tablespace_trlog(tablespace, &get->uuid);
    return get->trlog ? MDV_OK : MDV_FAILED;
}


static mdv_errno mdv_tablespace_evt_trlog_apply(void *arg, mdv_event *event)
{
    mdv_tablespace      *tablespace = arg;
    mdv_evt_trlog_apply *apply      = (mdv_evt_trlog_apply *)event;

    return mdv_tablespace_log_apply(tablespace, &apply->trlog)
                ? MDV_OK
                : MDV_FAILED;
}


static mdv_errno mdv_tablespace_evt_topology(void *arg, mdv_event *event)
{
    mdv_tablespace      *tablespace = arg;
    mdv_evt_topology    *topo = (mdv_evt_topology *)event;

    mdv_hashmap *idmap = mdv_tablespace_storage_ids(topo->topology);

    if (!idmap)
        return MDV_NO_MEM;

    mdv_errno err = mdv_safeptr_set(tablespace->storage_ids, idmap);

    mdv_hashmap_release(idmap);

    return err;
}


static const mdv_event_handler_type mdv_tablespace_handlers[] =
{
    { MDV_EVT_TABLE_GET,      mdv_tablespace_evt_get_table },
    { MDV_EVT_TABLE_CREATE,   mdv_tablespace_evt_create_table },
    { MDV_EVT_ROWDATA_INSERT, mdv_tablespace_evt_rowdata_insert },
    { MDV_EVT_TRLOG_GET,      mdv_tablespace_evt_trlog_get },
    { MDV_EVT_TRLOG_APPLY,    mdv_tablespace_evt_trlog_apply },
    { MDV_EVT_TOPOLOGY,       mdv_tablespace_evt_topology },
};


mdv_tablespace * mdv_tablespace_open(mdv_uuid const *uuid, mdv_ebus *ebus, mdv_topology *topology)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(8);

    mdv_tablespace *tablespace = mdv_alloc(sizeof(mdv_tablespace), "tablespace");

    if (!tablespace)
    {
        MDV_LOGE("No memory for new tablespace");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, tablespace, "tablespace");

    tablespace->uuid = *uuid;

    mdv_hashmap *idmap = mdv_tablespace_storage_ids(topology);

    if (!idmap)
    {
        MDV_LOGE("No memory for new tablespace");
        mdv_rollback(rollbacker);
        return 0;
    }

    tablespace->storage_ids = mdv_safeptr_create(idmap,
                                        (mdv_safeptr_retain_fn)mdv_hashmap_retain,
                                        (mdv_safeptr_release_fn)mdv_hashmap_release);

    if (!tablespace->storage_ids)
    {
        MDV_LOGE("Safe pointer creation failed");
        mdv_hashmap_release(idmap);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_safeptr_free, tablespace->storage_ids);

    mdv_hashmap_release(idmap);

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

        mdv_safeptr_free(tablespace->storage_ids);

        memset(tablespace, 0, sizeof(*tablespace));
        mdv_free(tablespace, "tablespace");
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

    return table;
}


static mdv_errno mdv_tablespace_log_rowset(mdv_tablespace *tablespace, mdv_uuid const *table_id, binn *rowset)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_trlog *trlog = mdv_tablespace_trlog_create(tablespace, &tablespace->uuid);

    if (!trlog)
    {
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_trlog_release, trlog);

    mdv_rowdata *rowdata = mdv_tablespace_rowdata_create(tablespace, table_id);

    if (!rowdata)
    {
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_rowdata_release, rowdata);

    uint64_t id = 0;

    mdv_errno err = mdv_rowdata_reserve(rowdata, mdv_binn_list_length(rowset), &id);

    if (err != MDV_OK)
    {
        mdv_rollback(rollbacker);
        return err;
    }

    int const binn_rowset_size = binn_size(rowset);

    size_t const op_size = offsetof(mdv_trlog_op, payload)
                            + sizeof id
                            + sizeof *table_id
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

    memcpy(payload, &id, sizeof id);                        payload += sizeof id;
    memcpy(payload, table_id, sizeof *table_id);            payload += sizeof *table_id;
    memcpy(payload, binn_ptr(rowset), binn_rowset_size);    payload += binn_rowset_size;

    if (!mdv_trlog_add_op(trlog, op))
    {
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollback(rollbacker);

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

    bool ret = true;

    switch(op->type)
    {
        case MDV_OP_TABLE_CREATE:
        {
            binn obj;

            if (!binn_load(op->payload, &obj))
            {
                MDV_LOGE("Invalid transaction operation");
                return false;
            }

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

            binn_free(&obj);

            break;
        }

        case MDV_OP_ROW_INSERT:
        {
            uint8_t *payload = op->payload;

            uint64_t id;
            mdv_uuid table_id;

            memcpy(&id, payload, sizeof id);                payload += sizeof id;
            memcpy(&table_id, payload, sizeof table_id);    payload += sizeof table_id;

            binn rowset;

            ret = binn_load(payload, &rowset);

            if (!ret)
            {
                MDV_LOGE("Invalid rowset");
                break;
            }

            mdv_rowdata *rowdata = mdv_tablespace_rowdata_create(tablespace, &table_id);

            if (rowdata)
            {
                binn_iter iter;
                binn item;

                binn_list_foreach(&rowset, item)
                {
                    mdv_objid const rowid =
                    {
                        .node = context->node_id,
                        .id = id++
                    };

                    mdv_data const row =
                    {
                        .size = binn_size(&item),
                        .ptr = binn_ptr(&item)
                    };

                    mdv_errno err = mdv_rowdata_add_raw(rowdata, &rowid, &row);

                    if (err != MDV_OK)
                    {
                        MDV_LOGE("Row insertion failed with error %d (%s)", err, mdv_strerror(err));
                        ret = false;
                        break;
                    }
                }

                mdv_rowdata_release(rowdata);
            }

            binn_free(&rowset);

            break;
        }

        default:
            MDV_LOGE("Unsupported DB operation");
    }

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
            .node_id = mdv_trlog_id(trlog)
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

