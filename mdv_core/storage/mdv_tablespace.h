#pragma once
#include "mdv_cfstorage.h"
#include <mdv_types.h>


typedef struct
{
    mdv_cfstorage cfstorage;
} mdv_tablespace;


#define mdv_tablespace_ok(storage) mdv_cfstorage_ok((storage).cfstorage)


mdv_tablespace mdv_tablespace_create();
mdv_tablespace mdv_tablespace_open();
bool           mdv_tablespace_drop();
void           mdv_tablespace_close(mdv_tablespace *tablespace);
int            mdv_tablespace_create_table(mdv_tablespace *tablespace, mdv_table_base *table);

