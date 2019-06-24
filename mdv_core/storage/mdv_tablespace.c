#include "mdv_tablespace.h"
#include <mdv_status.h>


static const mdv_uuid MDV_TABLES_UUID = {};


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
    return MDV_STATUS_NOT_IMPL;
}
