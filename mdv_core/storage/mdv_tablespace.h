#pragma once
#include "mdv_cfstorage.h"
#include <mdv_types.h>


typedef struct
{
    mdv_cfstorage *cfstorage;
} mdv_tablespace;


#define mdv_tablespace_ok(storage) ((storage).cfstorage != 0)


mdv_tablespace mdv_tablespace_create(uint32_t nodes_num);
mdv_tablespace mdv_tablespace_open(uint32_t nodes_num);
bool           mdv_tablespace_drop();
void           mdv_tablespace_close(mdv_tablespace *tablespace);
int            mdv_tablespace_create_table(mdv_tablespace *tablespace, mdv_table_base *table);

