#pragma once
#include "mdv_cfstorage.h"


typedef struct
{
    mdv_cfstorage cfstorage;
} mdv_tablespace;


#define mdv_tablespace_ok(storage) mdv_cfstorage_ok((storage).cfstorage)


mdv_tablespace mdv_tablespace_create();
mdv_tablespace mdv_tablespace_open();
bool           mdv_tablespace_drop();
void           mdv_tablespace_close(mdv_tablespace *tablespace);