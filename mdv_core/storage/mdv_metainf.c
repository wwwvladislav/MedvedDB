#include "mdv_metainf.h"


static const char MDV_STRG_TBL_METAINF[]    = "METAINF";
static const char MDV_STRG_KEY_NODE_UUID[]  = "NODE_UUID";


bool mdv_metainf_load(MDB_txn *txn, MDB_dbi dbi, mdv_metainf *metainf)
{
    return true;
}


bool mdv_metainf_save(MDB_txn *txn, MDB_dbi dbi, mdv_metainf *metainf)
{
    return true;
}
