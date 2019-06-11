#pragma once
#include <mdv_uuid.h>
#include <stdbool.h>
#include <lmdb.h>


typedef struct
{
    mdv_uuid uuid;

    uint8_t  uuid_changed:1;

    struct
    {
        MDB_env *env;
    } db;
} mdv_metainf;


bool mdv_metainf_load(MDB_txn *txn, MDB_dbi dbi, mdv_metainf *metainf);
bool mdv_metainf_save(MDB_txn *txn, MDB_dbi dbi, mdv_metainf *metainf);
