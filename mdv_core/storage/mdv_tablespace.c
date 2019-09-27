#include "mdv_tablespace.h"
#include <mdv_serialization.h>
#include <mdv_alloc.h>
#include <mdv_tracker.h>
#include <mdv_rollbacker.h>


/// Conflict-free Replicated Storage reference
typedef struct
{
    mdv_uuid       uuid;        ///< Storage UUID
    mdv_cfstorage *cfstorage;   ///< Conflict-free Replicated Storage
} mdv_cfstorage_ref;


/// Storage for DB tables
static const mdv_uuid MDV_DB_TABLES = {};


/// DB operations list
enum
{
    MDV_OP_TABLE_CREATE = 0,    ///< Create table
    MDV_OP_TABLE_DROP,          ///< Drop table
    MDV_OP_ROW_INSERT           ///< Insert data into a table
};


/// DB operation
typedef struct
{
    uint32_t op;                ///< operation id
    uint32_t alignment;         ///< alignment (unused)
    uint8_t  payload[1];        ///< operation payload
} mdv_op;


mdv_errno mdv_tablespace_open(mdv_tablespace *tablespace, uint32_t nodes_num)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_errno err = mdv_mutex_create(&tablespace->mutex);

    if (err != MDV_OK)
    {
        mdv_rollback(rollbacker);
        return err;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &tablespace->mutex);

    if (!mdv_hashmap_init(tablespace->storages,
                          mdv_cfstorage_ref,
                          uuid,
                          64,
                          mdv_uuid_hash,
                          mdv_uuid_cmp))
    {
        mdv_rollback(rollbacker);
        return MDV_NO_MEM;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &tablespace->storages);

    mdv_cfstorage_ref ref =
    {
        .uuid = MDV_DB_TABLES,
        .cfstorage = mdv_cfstorage_open(&MDV_DB_TABLES, nodes_num)
    };

    if (!ref.cfstorage)
    {
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_cfstorage_close, ref.cfstorage);

    if (!mdv_hashmap_insert(tablespace->storages, ref))
    {
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_free(rollbacker);

    return MDV_OK;
}


void mdv_tablespace_close(mdv_tablespace *tablespace)
{
    mdv_hashmap_foreach(tablespace->storages, mdv_cfstorage_ref, ref)
    {
        mdv_cfstorage_close(ref->cfstorage);
    }

    mdv_hashmap_free(tablespace->storages);

    mdv_mutex_free(&tablespace->mutex);
}


mdv_cfstorage * mdv_tablespace_cfstorage(mdv_tablespace *tablespace, mdv_uuid const *uuid)
{
    mdv_cfstorage *storage = 0;

    if (mdv_mutex_lock(&tablespace->mutex) == MDV_OK)
    {
        mdv_cfstorage_ref *ref = mdv_hashmap_find(tablespace->storages, *uuid);
        storage = ref ? ref->cfstorage : 0;
        mdv_mutex_unlock(&tablespace->mutex);
    }

    return storage;
}


mdv_rowid const * mdv_tablespace_create_table(mdv_tablespace *tablespace, mdv_table_base const *table)
{
    mdv_cfstorage *tables = mdv_tablespace_cfstorage(tablespace, &MDV_DB_TABLES);

    if (!tables)
        return 0;

    binn obj;

    if (!mdv_binn_table(table, &obj))
        return 0;

    int const obj_size = binn_size(&obj);

    size_t const op_size = offsetof(mdv_op, payload) + obj_size;

    mdv_op *op = (mdv_op*)mdv_staligned_alloc(sizeof(uint32_t), op_size, "DB op");

    if (!op)
    {
        binn_free(&obj);
        return 0;
    }

    op->op = MDV_OP_TABLE_CREATE;

    memcpy(op->payload, binn_ptr(&obj), obj_size);

    binn_free(&obj);

    mdv_cfstorage_op cfop =
    {
        .row_id = mdv_cfstorage_new_id(tables),
        .op =
        {
            .size = op_size,
            .ptr = op
        }
    };

    mdv_rowid const *rowid = 0;

    mdv_list ops = {};

    if (mdv_list_push_back(&ops, cfop)
        && mdv_cfstorage_log_add(tables, MDV_LOCAL_ID, &ops))
    {
        static _Thread_local mdv_rowid id;

        id.peer = MDV_LOCAL_ID;
        id.id = cfop.row_id;

        rowid = &id;
    }

    mdv_list_clear(&ops);

    mdv_stfree(op, "DB op");

    return rowid;
}


mdv_vector * mdv_tablespace_storages(mdv_tablespace *tablespace)
{
    mdv_vector *uuids = mdv_vector_create(54, sizeof(mdv_uuid), &mdv_default_allocator);

    if (!uuids)
        return 0;

    if (mdv_mutex_lock(&tablespace->mutex) == MDV_OK)
    {
        mdv_hashmap_foreach(tablespace->storages, mdv_cfstorage_ref, ref)
        {
            mdv_vector_push_back(uuids, &ref->uuid);
        }
        mdv_mutex_unlock(&tablespace->mutex);
    }

    return uuids;
}

