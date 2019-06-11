#include "mdv_storage.h"
#include <mdv_log.h>
#include <mdv_filesystem.h>


MDB_env * mdv_env_open(char const *path, uint32_t dbs_num, uint32_t flags)
{
    if (!path)
    {
        MDV_LOGE("DB path is empty");
        return 0;
    }

    if (!mdv_mkdir(path))
    {
        MDV_LOGE("DB storage directory creation failed: '%s'", path);
        return 0;
    }

    #define MDV_DB_CALL(expr)                                                               \
        if((rc = (expr)) != MDB_SUCCESS)                                                    \
        {                                                                                   \
            MDV_LOGE("The LMDB initialization failed: '%s' (%d)", mdb_strerror(rc), rc);    \
            mdb_env_close(env);                                                             \
            return 0;                                                                       \
        }

    MDB_env *env = 0;
    int rc = 0;

    MDV_DB_CALL(mdb_env_create(&env));
    MDV_DB_CALL(mdb_env_set_maxdbs(env, dbs_num));
    MDV_DB_CALL(mdb_env_open(env, path, flags, 0664));

    #undef MDV_DB_CALL

    return env;
}
