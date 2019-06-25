#include "mdv_tablespace.h"
#include <mdv_status.h>
#include <mdv_serialization.h>


static const mdv_uuid MDV_TABLES_UUID = {};


enum
{
    MDV_OP_TABLE_CREATE = 0
};


mdv_tablespace mdv_tablespace_create()
{
    mdv_tablespace tables =
    {
        .cfstorage = mdv_cfstorage_create(&MDV_TABLES_UUID)
    };
    return tables;
}


mdv_tablespace mdv_tablespace_open()
{
    mdv_tablespace tables =
    {
        .cfstorage = mdv_cfstorage_open(&MDV_TABLES_UUID)
    };
    return tables;
}


bool mdv_tablespace_drop()
{
    return mdv_cfstorage_drop(&MDV_TABLES_UUID);
}


void mdv_tablespace_close(mdv_tablespace *tablespace)
{
    mdv_cfstorage_close(&tablespace->cfstorage);
}


int mdv_tablespace_create_table(mdv_tablespace *tablespace, mdv_table_base *table)
{
    table->uuid = mdv_uuid_generate();

    binn obj;
    if (!mdv_binn_table(table, &obj))
        return MDV_STATUS_FAILED;

    int const obj_size = binn_size(&obj);
    uint8_t data[sizeof(uint32_t) + obj_size];
    *(uint32_t*)data = MDV_OP_TABLE_CREATE;
    memcpy(data + sizeof(uint32_t), binn_ptr(&obj), obj_size);

    mdv_cfstorage_op op =
    {
        .key =
        {
            .size = sizeof(table->uuid),
            .ptr = table->uuid.u8
        },
        .op =
        {
            .size = sizeof(data),
            .ptr = data
        }
    };

    int ret = mdv_cfstorage_add(&tablespace->cfstorage, 1, &op)
            ? MDV_STATUS_OK
            : MDV_STATUS_FAILED;

    binn_free(&obj);

    return ret;
}
