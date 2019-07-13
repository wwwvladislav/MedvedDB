#include "mdv_tablespace.h"
#include <mdv_serialization.h>
#include <mdv_alloc.h>


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


mdv_errno mdv_tablespace_create(mdv_tablespace *tablespace, uint32_t nodes_num)
{
    tablespace->tables = mdv_cfstorage_create(&MDV_DB_TABLES, nodes_num);

    mdv_errno err = tablespace->tables
                        ? MDV_OK
                        : MDV_FAILED;
    return err;
}


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


mdv_errno mdv_tablespace_log_create_table(mdv_tablespace *tablespace, uint32_t peer_id, mdv_table_base *table)
{
    table->uuid = mdv_uuid_generate();

    binn obj;
    if (!mdv_binn_table(table, &obj))
        return MDV_FAILED;

    int const obj_size = binn_size(&obj);

    mdv_op *op = (mdv_op*)mdv_alloc_tmp(offsetof(mdv_op, payload) + obj_size);

    if (!op)
    {
        binn_free(&obj);
        return MDV_NO_MEM;
    }

    op->op = MDV_OP_TABLE_CREATE;

    memcpy(op->payload, binn_ptr(&obj), obj_size);

    binn_free(&obj);

    mdv_cfstorage_op cfop =
    {
        .key =
        {
            .size = sizeof(table->uuid),
            .ptr = table->uuid.u8
        },
        .op =
        {
            .size = sizeof(op),
            .ptr = op
        }
    };

    mdv_errno ret = mdv_cfstorage_add(tablespace->tables, peer_id, 1, &cfop)
                        ? MDV_OK
                        : MDV_FAILED;

    mdv_free(op);

    return ret;
}
