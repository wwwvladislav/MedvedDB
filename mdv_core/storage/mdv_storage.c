#include "mdv_storage.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <stdatomic.h>
#include <lmdb.h>


struct mdv_storage
{
    atomic_int  ref_counter;
    MDB_env    *env;
};


mdv_storage * mdv_storage_open(char const *path, uint32_t dbs_num, uint32_t flags)
{
    if (!path)
    {
        MDV_LOGE("DB path is empty");
        return 0;
    }

    #define MDV_DB_CALL(expr)                                                               \
        if((rc = (expr)) != MDB_SUCCESS)                                                    \
        {                                                                                   \
            MDV_LOGE("The LMDB initialization failed: '%s' (%d)", mdb_strerror(rc), rc);    \
            mdb_env_close(env);                                                             \
            env = 0;                                                                        \
            return 0;                                                                       \
        }

    int rc = 0;
    MDB_env *env = 0;

    MDV_DB_CALL(mdb_env_create(&env));
    MDV_DB_CALL(mdb_env_set_maxdbs(env, dbs_num));
    MDV_DB_CALL(mdb_env_open(env, path, flags, 0664));

    #undef MDV_DB_CALL

    mdv_storage *pstorage = (mdv_storage *)mdv_alloc(sizeof(mdv_storage));
    if (!pstorage)
    {
        MDV_LOGE("Memory allocation failed");
        mdb_env_close(env);
    }

    atomic_init(&pstorage->ref_counter, 1);
    pstorage->env = env;

    return pstorage;
}


mdv_storage * mdv_storage_retain(mdv_storage *pstorage)
{
    if (pstorage)
        atomic_fetch_add_explicit(&pstorage->ref_counter, 1, memory_order_relaxed);
    return pstorage;
}


void mdv_storage_release(mdv_storage *pstorage)
{
    if (pstorage)
    {
        if (atomic_fetch_sub_explicit(&pstorage->ref_counter, 1, memory_order_relaxed) == 1)
        {
            mdb_env_close(pstorage->env);
            mdv_free(pstorage);
        }
    }
}


bool mdv_transaction_start(mdv_storage *pstorage, mdv_transaction *ptransaction)
{
    MDB_txn *txn;

    int rc = mdb_txn_begin(pstorage->env, 0, 0, &txn);

    if(rc != MDB_SUCCESS)
    {
        MDV_LOGE("The LMDB transaction wasn't started: '%s' (%d)", mdb_strerror(rc), rc);
        return false;
    }

    ptransaction->pstorage = mdv_storage_retain(pstorage);
    ptransaction->ptransaction = txn;

    return true;
}


bool mdv_transaction_commit(mdv_transaction *ptransaction)
{
    MDB_txn *txn = (MDB_txn*)ptransaction->ptransaction;

    if (txn)
    {
        int rc = mdb_txn_commit(txn);
        if(rc != MDB_SUCCESS)
            MDV_LOGE("The LMDB transaction wasn't committed: '%s' (%d)", mdb_strerror(rc), rc);

        mdv_storage_release(ptransaction->pstorage);

        ptransaction->ptransaction = 0;
        ptransaction->pstorage = 0;

        return rc == MDB_SUCCESS;
    }

    return false;
}


bool mdv_transaction_abort(mdv_transaction *ptransaction)
{
    MDB_txn *txn = (MDB_txn*)ptransaction->ptransaction;

    if (txn)
    {
        mdb_txn_abort(txn);

        mdv_storage_release(ptransaction->pstorage);

        ptransaction->ptransaction = 0;
        ptransaction->pstorage = 0;

        return true;
    }

    return false;
}


bool mdv_map_open(mdv_transaction *ptransaction, char const *name, uint32_t flags, mdv_map *pmap)
{
    uint32_t mdb_flags = 0;
    if (flags & MDV_MAP_CREATE)           mdb_flags |= MDB_CREATE;
    if (flags & MDV_MAP_MULTI)            mdb_flags |= MDB_DUPSORT;
    if (flags & MDV_MAP_INTEGERKEY)       mdb_flags |= MDB_INTEGERKEY;
    if (flags & MDV_MAP_FIXED_SIZE_VALUE) mdb_flags |= MDB_DUPFIXED;
    if (flags & MDV_MAP_INTEGERVAL)       mdb_flags |= MDB_INTEGERDUP;

    MDB_txn *txn = (MDB_txn*)ptransaction->ptransaction;
    MDB_dbi dbi;
    int rc = mdb_dbi_open(txn, name, mdb_flags, &dbi);
    if(rc != MDB_SUCCESS)
    {
        MDV_LOGE("Unable to open the LMDB database: '%s' (%d)", mdb_strerror(rc), rc);
        return false;
    }

    pmap->pstorage = mdv_storage_retain(ptransaction->pstorage);
    pmap->dbmap = dbi;

    return true;
}


void mdv_map_close(mdv_map *pmap)
{
    if (pmap)
    {
        MDB_dbi dbi = (MDB_dbi)pmap->dbmap;
        if (pmap->pstorage && dbi)
        {
            mdb_dbi_close(pmap->pstorage->env, dbi);
            mdv_storage_release(pmap->pstorage);
            pmap->pstorage = 0;
        }
    }
}


static bool mdv_map_put_impl(mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data const *value, unsigned int flags)
{
    MDB_txn *txn = (MDB_txn*)ptransaction->ptransaction;
    MDB_dbi dbi = (MDB_dbi)pmap->dbmap;

    if (!txn)
    {
        MDV_LOGE("Invalid operation. The data should be inserted in transaction.");
        return false;
    }

    MDB_val k = { key->size, key->data };
    MDB_val v = { value->size, value->data };

    int rc = mdb_put(txn, dbi, &k, &v, flags);
    if(rc != MDB_SUCCESS)
    {
        MDV_LOGE("Unable to put data into the LMDB database: '%s' (%d)", mdb_strerror(rc), rc);
        return false;
    }

    return true;
}


bool mdv_map_put(mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data const *value)
{
    return mdv_map_put_impl(pmap, ptransaction, key, value, 0);
}


bool mdv_map_put_unique(mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data const *value)
{
    return mdv_map_put_impl(pmap, ptransaction, key, value, MDB_NOOVERWRITE);
}


bool mdv_map_get(mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data *value)
{
    MDB_txn *txn = (MDB_txn*)ptransaction->ptransaction;
    MDB_dbi dbi = (MDB_dbi)pmap->dbmap;

    if (!txn)
    {
        MDV_LOGE("Invalid operation. The data should be inserted in transaction.");
        return false;
    }

    MDB_val k = { key->size, key->data };

    int rc = mdb_get(txn, dbi, &k, (MDB_val*)value);

    if (rc == MDB_NOTFOUND)
        return false;

    if(rc != MDB_SUCCESS)
    {
        MDV_LOGE("Unable to get data from LMDB database: '%s' (%d)", mdb_strerror(rc), rc);
        return false;
    }

    return true;
}


bool mdv_map_del(mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data const *value)
{
    MDB_txn *txn = (MDB_txn*)ptransaction->ptransaction;
    MDB_dbi dbi = (MDB_dbi)pmap->dbmap;

    if (!txn)
    {
        MDV_LOGE("Invalid operation. The data should be inserted in transaction.");
        return false;
    }

    MDB_val k = { key->size, key->data };
    MDB_val v = { value->size, value->data };

    int rc = mdb_del(txn, dbi, &k, &v);

    if (rc == MDB_NOTFOUND)
        return false;

    if(rc != MDB_SUCCESS)
    {
        MDV_LOGE("Unable to delete data from LMDB database: '%s' (%d)", mdb_strerror(rc), rc);
        return false;
    }

    return true;
}


bool mdv_cursor_open(mdv_map *pmap, mdv_transaction *ptransaction, mdv_cursor *pcursor)
{
    MDB_txn *txn = (MDB_txn*)ptransaction->ptransaction;
    MDB_dbi dbi = pmap->dbmap;
    MDB_cursor *cursor;

    int rc = mdb_cursor_open(txn, dbi, &cursor);

    if(rc != MDB_SUCCESS)
    {
        MDV_LOGE("Unable to open new cursor: '%s' (%d)", mdb_strerror(rc), rc);
        return false;
    }

    pcursor->pstorage = mdv_storage_retain(ptransaction->pstorage);
    pcursor->pcursor = cursor;

    return true;
}


void mdv_cursor_close(mdv_cursor *pcursor)
{
    MDB_cursor *cursor = pcursor->pcursor;
    mdb_cursor_close(cursor);
    mdv_storage_release(pcursor->pstorage);
    pcursor->pstorage = 0;
    pcursor->pcursor = 0;
}


bool mdv_cursor_get(mdv_cursor *pcursor, mdv_data *key, mdv_data *value, mdv_cursor_op op)
{
    MDB_cursor_op const cursor_op = op == MDV_CURSOR_FIRST ? MDB_FIRST :
                                    op == MDV_CURSOR_CURRENT ? MDB_GET_CURRENT :
                                    op == MDV_CURSOR_LAST ? MDB_LAST :
                                    op == MDV_CURSOR_LAST_DUP ? MDB_LAST_DUP :
                                    op == MDV_CURSOR_NEXT ? MDB_NEXT :
                                    op == MDV_CURSOR_NEXT_DUP ? MDB_NEXT_DUP :
                                    op == MDV_CURSOR_PREV ? MDB_PREV :
                                    op == MDV_CURSOR_SET ? MDB_SET :
                                    MDB_FIRST;
    MDB_cursor *cursor = (MDB_cursor *)pcursor->pcursor;

    int rc = mdb_cursor_get(cursor, (MDB_val*)key, (MDB_val*)value, cursor_op);

    switch(rc)
    {
        case MDB_SUCCESS:
            return true;
        case MDB_NOTFOUND:
            return false;
    }

    MDV_LOGE("Unable to get data by cursor: '%s' (%d)", mdb_strerror(rc), rc);

    return false;
}

