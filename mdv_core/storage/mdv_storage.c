#include "mdv_storage.h"
#include <mdv_log.h>
#include <mdv_filesystem.h>


mdv_storage mdv_storage_open(char const *path, uint32_t dbs_num, uint32_t flags)
{
    mdv_storage storage = {};

    if (!path)
    {
        MDV_LOGE("DB path is empty");
        return storage;
    }

    if (!mdv_mkdir(path))
    {
        MDV_LOGE("DB storage directory creation failed: '%s'", path);
        return storage;
    }

    #define MDV_DB_CALL(expr)                                                               \
        if((rc = (expr)) != MDB_SUCCESS)                                                    \
        {                                                                                   \
            MDV_LOGE("The LMDB initialization failed: '%s' (%d)", mdb_strerror(rc), rc);    \
            mdb_env_close(storage.env);                                                     \
            storage.env = 0;                                                                \
            return storage;                                                                 \
        }

    int rc = 0;

    MDV_DB_CALL(mdb_env_create(&storage.env));
    MDV_DB_CALL(mdb_env_set_maxdbs(storage.env, dbs_num));
    MDV_DB_CALL(mdb_env_open(storage.env, path, flags, 0664));

    #undef MDV_DB_CALL

    return storage;
}


void mdv_storage_close(mdv_storage *db)
{
    mdb_env_close(db->env);
    db->env = 0;
}

