#include "mdv_tablespace.h"
#include <mdv_serialization.h>
#include <mdv_alloc.h>
#include <mdv_tracker.h>


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
    tablespace->tables = mdv_cfstorage_open(&MDV_DB_TABLES, nodes_num);

    mdv_errno err = tablespace->tables
                        ? MDV_OK
                        : MDV_FAILED;
    return err;
}


mdv_errno mdv_tablespace_drop()
{
    return mdv_cfstorage_drop(&MDV_DB_TABLES)
                ? MDV_OK
                : MDV_FAILED;
}


void mdv_tablespace_close(mdv_tablespace *tablespace)
{
    mdv_cfstorage_close(tablespace->tables);
    tablespace->tables = 0;
}


mdv_rowid const * mdv_tablespace_create_table(mdv_tablespace *tablespace, mdv_table_base const *table)
{
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
        .row_id = mdv_cfstorage_new_id(tablespace->tables, MDV_LOCAL_ID),
        .op =
        {
            .size = op_size,
            .ptr = op
        }
    };

    mdv_rowid const *rowid = 0;

    if (mdv_cfstorage_log_add(tablespace->tables, MDV_LOCAL_ID, 1, &cfop))
    {
        static _Thread_local mdv_rowid id;

        id.peer = MDV_LOCAL_ID;
        id.id = cfop.row_id;

        rowid = &id;
    }

    mdv_stfree(op, "DB op");

    return rowid;
}
