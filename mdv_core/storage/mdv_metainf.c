#include "mdv_metainf.h"


static const char MDV_STRG_TBL_METAINF[]    = "METAINF";
static const char MDV_STRG_KEY_NODE_UUID[]  = "NODE_UUID";


mdv_metainf mdv_storage_metainf_load(mdv_storage *storage, MDB_txn *txn, MDB_dbi dbi)
{
    mdv_metainf metainf;
    return metainf;
}


bool mdv_storage_metainf_sync(mdv_storage *storage, MDB_txn *txn, MDB_dbi dbi, mdv_metainf *metainf)
{
    return true;
}
