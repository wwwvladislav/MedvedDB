#include "mdv_metainf.h"
#include "mdv_storages.h"
#include <mdv_log.h>
#include <mdv_string.h>


void mdv_metainf_init(mdv_metainf *m)
{
    mdv_map_field_init(m->version,      0);
    mdv_map_field_init_last(m->uuid,    1);
}


mdv_storage * mdv_metainf_storage_open(char const *path)
{
    mdv_storage *storage = mdv_storage_open(path, MDV_STRG_METAINF, MDV_STRG_METAINF_MAPS, MDV_STRG_NOSUBDIR);

    if (!storage)
    {
        MDV_LOGE("The metainf storage initialization failed");
        return 0;
    }

    return storage;
}


bool mdv_metainf_load(mdv_metainf *metainf, mdv_storage *storage)
{
    mdv_metainf_init(metainf);

    // Load metainf
    mdv_transaction transaction = mdv_transaction_start(storage);

    if (mdv_transaction_ok(transaction))
    {
        mdv_map map = mdv_map_open(&transaction, MDV_MAP_METAINF, MDV_MAP_INTEGERKEY);

        if (mdv_map_ok(map))
            mdv_map_read(&map, &transaction, metainf);

        mdv_transaction_abort(&transaction);

        mdv_map_close(&map);
    }

    return true;
}


void mdv_metainf_validate(mdv_metainf *metainf)
{
    if (metainf->version.m.empty)
        mdv_map_field_set(metainf->version, MDV_VERSION);
    if (metainf->uuid.m.empty)
        mdv_map_field_set(metainf->uuid, mdv_uuid_generate());
}


void mdv_metainf_flush(mdv_metainf *metainf, mdv_storage *storage)
{
    // Save metainf
    mdv_transaction transaction = mdv_transaction_start(storage);
    if (mdv_transaction_ok(transaction))
    {
        mdv_map map = mdv_map_open(&transaction, MDV_MAP_METAINF, MDV_MAP_CREATE | MDV_MAP_INTEGERKEY);
        if (mdv_map_ok(map))
            mdv_map_write(&map, &transaction, metainf);
        mdv_transaction_commit(&transaction);
        mdv_map_close(&map);
    }
}
