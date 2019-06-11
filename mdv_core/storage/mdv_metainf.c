#include "mdv_metainf.h"
#include <lmdb.h>
#include <mdv_filesystem.h>
#include <mdv_log.h>
#include <mdv_string.h>


static const size_t MDV_METAINF_DBS = 2;

static const char MDV_TBL_METAINF[]         = "METAINF";
static const uint32_t MDV_KEY_NODE_UUID     = 0;

static const char MDV_TBL_TABLES[]          = "TABLES";
static const uint32_t MDV_KEY_NAME          = 0;


static struct
{
    MDB_env *env;
} db;


static bool mdv_metainf_load(mdv_metainf *metainf)
{
    MDB_txn *txn = 0;

    // Create transaction
    int rc = mdb_txn_begin(db.env, 0, 0, &txn);
    if(rc != MDB_SUCCESS)
    {
        MDV_LOGE("The LMDB transaction wasn't started: '%s'", mdb_strerror(rc));
        return false;
    }


    // Open "METAINF" table
    MDB_dbi dbi = 0;
    rc = mdb_dbi_open(txn, MDV_TBL_METAINF, MDB_CREATE | MDB_INTEGERKEY, &dbi);
    if(rc != MDB_SUCCESS)
    {
        MDV_LOGE("The LMDB table '%s' wasn't opened: '%s'", MDV_TBL_METAINF, mdb_strerror(rc));
        mdb_txn_abort(txn);
        return false;
    }


    // Search NODE_UUID
    uint32_t key_id = MDV_KEY_NODE_UUID;
    MDB_val key = { sizeof(key_id), &key_id };
    MDB_val value = { sizeof(metainf->uuid), &metainf->uuid };

    rc = mdb_get(txn, dbi, &key, &value);
    if (rc == MDB_NOTFOUND)
    {
        // If isn't found generate new one
        metainf->uuid = mdv_uuid_generate();

        // Save generated node UUID
        rc = mdb_put(txn, dbi, &key, &value, MDB_NODUPDATA);
        if(rc != MDB_SUCCESS)
        {
            MDV_LOGE("mdb_put failed: %s", mdb_strerror(rc));
            mdb_dbi_close(db.env, dbi);
            mdb_txn_abort(txn);
            return false;
        }
    }
    else
        metainf->uuid = *(mdv_uuid const*)value.mv_data;

    // Commit transaction
    mdb_txn_commit(txn);
    mdb_dbi_close(db.env, dbi);


    // Print node UUID to log
    char tmp[33];
    mdv_string uuid_str = mdv_str_static(tmp);
    if (mdv_uuid_to_str(&metainf->uuid, &uuid_str))
        MDV_LOGI("Node UUID: %s", uuid_str.ptr);

    return true;
}


bool mdv_metainf_open(mdv_metainf *metainf, char const *path)
{
    if (db.env)
        return true;


    // Get metainf file path
    mdv_stack(char, 1024) mpool;
    mdv_stack_clear(mpool);

    mdv_string db_path = mdv_str_pdup(mpool, path);

    if (mdv_str_empty(db_path))
    {
        MDV_LOGE("Metainf storage directory length is too long: '%s'", path);
        return false;
    }

    mdv_string const metainf_file = mdv_str_static("/metainf.mdb");

    db_path = mdv_str_pcat(mpool, db_path, metainf_file);

    if (mdv_str_empty(db_path))
    {
        MDV_LOGE("Metainf storage directory length is too long: '%s'", path);
        return false;
    }


    // Create DB root directory
    if (!mdv_mkdir(path))
    {
        MDV_LOGE("Metainf storage directory creation failed: '%s'", path);
        return false;
    }


    // Create LMDB environment
    #define MDV_DB_CALL(expr)                                                               \
        if((rc = (expr)) != MDB_SUCCESS)                                                    \
        {                                                                                   \
            MDV_LOGE("The LMDB initialization failed: '%s' (%d)", mdb_strerror(rc), rc);    \
            mdb_env_close(env);                                                             \
            return false;                                                                   \
        }

    MDB_env *env = 0;
    int rc = 0;

    MDV_DB_CALL(mdb_env_create(&env));
    MDV_DB_CALL(mdb_env_set_maxdbs(env, MDV_METAINF_DBS));
    MDV_DB_CALL(mdb_env_open(env, db_path.ptr, MDB_NOSUBDIR, 0664));

    #undef MDV_DB_CALL

    db.env = env;

    // Load meta info
    return mdv_metainf_load(metainf);
}


void mdv_metainf_close()
{
    mdb_env_close(db.env);
    db.env = 0;
}
