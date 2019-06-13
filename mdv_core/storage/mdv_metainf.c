#include "mdv_metainf.h"
#include "mdv_storage.h"
#include <mdv_filesystem.h>
#include <mdv_log.h>
#include <mdv_string.h>


static const size_t MDV_METAINF_DBS = 2;
static const char MDV_TBL_METAINF[]         = "METAINF";
static const char MDV_TBL_TABLES[]          = "TABLES";


static struct
{
    mdv_storage *storage;
} db;

void mdv_metainf_init(mdv_metainf *m)
{
    mdv_map_field_init(m->version,      0);
    mdv_map_field_init_last(m->uuid,    1);
}


bool mdv_metainf_open(mdv_metainf *metainf, char const *path)
{
    // Open storage
    if (!db.storage)
    {
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


        db.storage = mdv_storage_open(db_path.ptr, MDV_METAINF_DBS, MDV_STRG_NOSUBDIR);
        if (!db.storage)
        {
            MDV_LOGE("The metainf storage initialization failed");
            return false;
        }
    }

    mdv_metainf_init(metainf);

    // Load metainf
    mdv_transaction transaction = mdv_transaction_start(db.storage);
    if (mdv_transaction_ok(transaction))
    {
        mdv_map map = mdv_map_open(&transaction, MDV_TBL_METAINF, MDV_MAP_INTEGERKEY);
        if (mdv_map_ok(map))
            mdv_map_read(&map, &transaction, metainf);
        mdv_transaction_abort(&transaction);
        mdv_map_close(&map);
    }

    return true;
}


void mdv_metainf_close()
{
    mdv_storage_release(db.storage);
    db.storage = 0;
}


void mdv_metainf_validate(mdv_metainf *metainf)
{
    if (metainf->version.m.empty)
        mdv_map_field_set(metainf->version, MDV_VERSION);
    if (metainf->uuid.m.empty)
        mdv_map_field_set(metainf->uuid, mdv_uuid_generate());
}


void mdv_metainf_flush(mdv_metainf *metainf)
{
    // Save metainf
    mdv_transaction transaction = mdv_transaction_start(db.storage);
    if (mdv_transaction_ok(transaction))
    {
        mdv_map map = mdv_map_open(&transaction, MDV_TBL_METAINF, MDV_MAP_CREATE | MDV_MAP_INTEGERKEY);
        if (mdv_map_ok(map))
            mdv_map_write(&map, &transaction, metainf);
        mdv_transaction_commit(&transaction);
        mdv_map_close(&map);
    }
}
