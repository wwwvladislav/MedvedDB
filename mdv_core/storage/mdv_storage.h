#pragma once
#include <lmdb.h>


MDB_env * mdv_env_open(char const *path, uint32_t dbs_num, uint32_t flags);

#define mdv_env_close mdb_env_close


typedef struct
{
    MDB_env            *env;
} mdv_commit_log;

typedef struct
{
    struct
    {
        MDB_env        *env;
    } data;

    struct
    {
        mdv_commit_log  add;
        mdv_commit_log  del;
    } log;
} mdv_table;
