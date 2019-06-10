#pragma once
#include <mdv_uuid.h>
#include <stdbool.h>
#include "mdv_storage.h"


typedef struct
{
    mdv_uuid uuid;

    uint8_t  uuid_changed:1;
} mdv_metainf;


mdv_metainf mdv_storage_metainf_load(mdv_storage *storage, MDB_txn *txn, MDB_dbi dbi);
bool        mdv_storage_metainf_sync(mdv_storage *storage, MDB_txn *txn, MDB_dbi dbi, mdv_metainf *metainf);
