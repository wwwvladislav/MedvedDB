#include "mdv_lmdb.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <mdv_string.h>
#include <mdv_filesystem.h>
#include <stdatomic.h>
#include <string.h>
#include <assert.h>

#ifndef _LMDB_H_
#define MDB_VL32    // LMDB must use uint64_t for keys
#include <lmdb.h>
#else
#error "Only mdv_storage must be used for LMDB access"
#endif


/// @cond Doxygen_Suppress

struct mdv_lmdb
{
    atomic_uint_fast32_t    ref_counter;
    MDB_env                *env;
};

/// @endcond


mdv_lmdb * mdv_storage_open(char const *path, char const *name, uint32_t dbs_num, uint32_t flags, size_t mapsize)
{
    if (!path || !name)
    {
        MDV_LOGE("DB path is empty");
        return 0;
    }

    mdv_stack(char, 1024) mpool;
    mdv_stack_clear(mpool);

    mdv_string db_path = mdv_str_pdup(mpool, path);

    if (mdv_str_empty(db_path))
    {
        MDV_LOGE("Storage directory length is too long: '%s'", path);
        return 0;
    }

    mdv_string const delimiter = mdv_str_static("/");
    mdv_string const file_name = mdv_str((char*)name);

    db_path = mdv_str_pcat(mpool, db_path, delimiter);
    db_path = mdv_str_pcat(mpool, db_path, file_name);

    if (mdv_str_empty(db_path))
    {
        MDV_LOGE("Storage directory length is too long: '%s'", name);
        return 0;
    }

    // Create DB root directory
    if (!mdv_mkdir(path))
    {
        MDV_LOGE("Storage directory creation failed: '%s'", path);
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

    uint32_t mdb_flags = MDB_NOSUBDIR;

    if (flags & MDV_STRG_FIXEDMAP)      mdb_flags |= MDB_FIXEDMAP;
    if (flags & MDV_STRG_NOSUBDIR)      mdb_flags |= MDB_NOSUBDIR;
    if (flags & MDV_STRG_RDONLY)        mdb_flags |= MDB_RDONLY;
    if (flags & MDV_STRG_WRITEMAP)      mdb_flags |= MDB_WRITEMAP;
    if (flags & MDV_STRG_NOMETASYNC)    mdb_flags |= MDB_NOMETASYNC;
    if (flags & MDV_STRG_NOSYNC)        mdb_flags |= MDB_NOSYNC;
    if (flags & MDV_STRG_MAPASYNC)      mdb_flags |= MDB_MAPASYNC;
    if (flags & MDV_STRG_NOTLS)         mdb_flags |= MDB_NOTLS;
    if (flags & MDV_STRG_NOLOCK)        mdb_flags |= MDB_NOLOCK;
    if (flags & MDV_STRG_NORDAHEAD)     mdb_flags |= MDB_NORDAHEAD;
    if (flags & MDV_STRG_NOMEMINIT)     mdb_flags |= MDB_NOMEMINIT;

    int rc = 0;
    MDB_env *env = 0;

    MDV_DB_CALL(mdb_env_create(&env));
    MDV_DB_CALL(mdb_env_set_maxdbs(env, dbs_num));
    MDV_DB_CALL(mdb_env_set_mapsize(env, mapsize));
    MDV_DB_CALL(mdb_env_open(env, db_path.ptr, mdb_flags, 0664));

    #undef MDV_DB_CALL

    mdv_lmdb *pstorage = (mdv_lmdb *)mdv_alloc(sizeof(mdv_lmdb), "storage");
    if (!pstorage)
    {
        MDV_LOGE("Memory allocation failed");
        mdb_env_close(env);
    }

    atomic_init(&pstorage->ref_counter, 1);
    pstorage->env = env;

    return pstorage;
}


mdv_lmdb * mdv_storage_retain(mdv_lmdb *pstorage)
{
    if (pstorage)
        atomic_fetch_add_explicit(&pstorage->ref_counter, 1, memory_order_relaxed);
    return pstorage;
}


uint32_t mdv_storage_release(mdv_lmdb *pstorage)
{
    uint32_t rc = 0;

    if (pstorage)
    {
        rc = atomic_fetch_sub_explicit(&pstorage->ref_counter, 1, memory_order_relaxed) - 1;

        if (!rc)
        {
            mdb_env_close(pstorage->env);
            mdv_free(pstorage, "storage");
        }
    }

    return rc;
}


mdv_transaction mdv_transaction_start(mdv_lmdb *pstorage)
{
    MDB_txn *txn;

    int rc = mdb_txn_begin(pstorage->env, 0, 0, &txn);

    if(rc != MDB_SUCCESS)
    {
        MDV_LOGE("The LMDB transaction wasn't started: '%s' (%d)", mdb_strerror(rc), rc);
        return (mdv_transaction){ 0, 0 };
    }

    return (mdv_transaction){ mdv_storage_retain(pstorage), txn };
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


mdv_map mdv_map_open(mdv_transaction *ptransaction, char const *name, uint32_t flags)
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
        if (!(MDV_MAP_SILENT & flags))
            MDV_LOGE("Unable to open the LMDB database: '%s' (%d)", mdb_strerror(rc), rc);
        return (mdv_map){ 0, 0 };
    }

    return (mdv_map){ mdv_storage_retain(ptransaction->pstorage), dbi };
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


enum { MDV_HEXSTR_LEN = 65 };

static char const * mdv_array_to_hexstr(uint8_t const *ptr, size_t size, char buff[MDV_HEXSTR_LEN])
{
    size *= 2;

    size = size > MDV_HEXSTR_LEN - 1 ? MDV_HEXSTR_LEN - 1 : size;

    static char const hex[] = "0123456789ABCDEF";

    for(size_t i = 0; i < size; ++i)
    {
        uint8_t const d = (ptr[i / 2] >> ((1 - i % 2) * 4)) & 0xF;
        buff[i] = hex[d];
    }

    buff[size] = 0;

    return buff;
}


static bool mdv_map_put_impl(mdv_map *pmap, mdv_transaction *ptransaction, mdv_data const *key, mdv_data const *value, unsigned int flags)
{
    MDB_txn *txn = (MDB_txn*)ptransaction->ptransaction;
    MDB_dbi dbi = (MDB_dbi)pmap->dbmap;

    if (!txn)
    {
        if (!(MDV_MAP_SILENT & flags))
            MDV_LOGE("Invalid operation. The data should be inserted in transaction.");
        return false;
    }

    MDB_val k = { key->size, key->ptr };
    MDB_val v = { value->size, value->ptr };

    int rc = mdb_put(txn, dbi, &k, &v, flags);

    if(rc != MDB_SUCCESS)
    {
        if (!(MDV_MAP_SILENT & flags))
        {
            char hexstr[MDV_HEXSTR_LEN];

            MDV_LOGE("Unable to put data with key \'%s\' into the LMDB database: '%s' (%d)",
                            mdv_array_to_hexstr(key->ptr, key->size, hexstr),
                            mdb_strerror(rc),
                            rc);
        }
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

    MDB_val k = { key->size, key->ptr };

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

    MDB_val k = { key->size, key->ptr };
    MDB_val v = { value ? value->size : 0, value ? value->ptr : 0 };

    int rc = mdb_del(txn, dbi, &k, value ? &v : 0);

    if (rc == MDB_NOTFOUND)
        return false;

    if(rc != MDB_SUCCESS)
    {
        MDV_LOGE("Unable to delete data from LMDB database: '%s' (%d)", mdb_strerror(rc), rc);
        return false;
    }

    return true;
}


mdv_cursor mdv_cursor_open(mdv_map *pmap, mdv_transaction *ptransaction)
{
    MDB_txn *txn = (MDB_txn*)ptransaction->ptransaction;
    MDB_dbi dbi = pmap->dbmap;
    MDB_cursor *cursor;

    int rc = mdb_cursor_open(txn, dbi, &cursor);

    if(rc != MDB_SUCCESS)
    {
        MDV_LOGE("Unable to open new cursor: '%s' (%d)", mdb_strerror(rc), rc);
        return (mdv_cursor){ 0, 0 };
    }

    return (mdv_cursor){ mdv_storage_retain(ptransaction->pstorage), cursor };
}

mdv_cursor mdv_cursor_open_explicit(mdv_map *pmap, mdv_transaction *ptransaction, mdv_data *key, mdv_data *value, mdv_cursor_op op)
{
    mdv_cursor cursor = mdv_cursor_open(pmap, ptransaction);

    if (!mdv_cursor_ok(cursor))
        return cursor;

    if (!mdv_cursor_get(&cursor, key, value, op))
    {
        mdv_cursor_close(&cursor);
        return (mdv_cursor){ 0, 0 };
    }

    return cursor;
}


mdv_cursor mdv_cursor_open_first(mdv_map *pmap, mdv_transaction *ptransaction, mdv_data *key, mdv_data *value)
{
    return mdv_cursor_open_explicit(pmap, ptransaction, key, value, MDV_CURSOR_FIRST);
}


bool mdv_cursor_close(mdv_cursor *pcursor)
{
    MDB_cursor *cursor = pcursor->pcursor;
    mdb_cursor_close(cursor);
    mdv_storage_release(pcursor->pstorage);
    pcursor->pstorage = 0;
    pcursor->pcursor = 0;
    return true;
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
                                    op == MDV_CURSOR_SET_RANGE ? MDB_SET_RANGE :
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


void mdv_map_read(mdv_map *pmap, mdv_transaction *ptransaction, void *map_fields)
{
    for(mdv_map_field_desc *field = (mdv_map_field_desc *)map_fields;
        field;
        field = field->next)
    {
        mdv_data value = {};
        if (mdv_map_get(pmap, ptransaction, &field->key, &value))
        {
            assert(value.size <= field->value.size);
            if (value.size <= field->value.size)
            {
                field->value.size = value.size;
                memcpy(field->value.ptr, value.ptr, value.size);
                field->empty = 0;
                field->changed = 0;
            }
            else
                MDV_LOGE("Map field is too long. Skipped.");
        }
    }
}


void mdv_map_write(mdv_map *pmap, mdv_transaction *ptransaction, void *map_fields)
{
    for(mdv_map_field_desc *field = (mdv_map_field_desc *)map_fields;
        field;
        field = field->next)
    {
        if (field->changed && mdv_map_put(pmap, ptransaction, &field->key, &field->value))
            field->changed = 0;
    }
}
