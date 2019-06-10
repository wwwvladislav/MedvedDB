#pragma once
#include <lmdb.h>


typedef struct
{
    MDB_env *env;
} mdv_storage;


mdv_storage mdv_storage_open(char const *path, uint32_t dbs_num, uint32_t flags);
void        mdv_storage_close(mdv_storage *db);
