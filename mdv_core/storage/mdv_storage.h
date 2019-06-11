#pragma once


typedef struct
{
    //MDB_env            *env;
} mdv_commit_log;


typedef struct
{
    struct
    {
        //MDB_env        *env;
    } data;

    struct
    {
        mdv_commit_log  add;
        mdv_commit_log  del;
    } log;
} mdv_table;
