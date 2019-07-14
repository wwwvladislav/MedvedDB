#include "mdv_service.h"
#include <mdv_log.h>
#include <mdv_threads.h>
#include <mdv_binn.h>


enum { MDV_NODES_NUM = 256 };


static void mdv_service_configure_logging()
{
    if (!mdv_str_empty(MDV_CONFIG.log.level))
    {
        switch(*MDV_CONFIG.log.level.ptr)
        {
            case 'f':   mdv_logf_set_level(ZF_LOG_FATAL);   break;
            case 'e':   mdv_logf_set_level(ZF_LOG_ERROR);   break;
            case 'w':   mdv_logf_set_level(ZF_LOG_WARN);    break;
            case 'i':   mdv_logf_set_level(ZF_LOG_INFO);    break;
            case 'd':   mdv_logf_set_level(ZF_LOG_DEBUG);   break;
            case 'v':   mdv_logf_set_level(ZF_LOG_VERBOSE); break;
            case 'n':   mdv_logf_set_level(ZF_LOG_NONE);    break;
        }
    }
}


bool mdv_service_init(mdv_service *svc, char const *cfg_file_path)
{
    // Configuration
    if (!mdv_load_config(cfg_file_path))
    {
        MDV_LOGE("Service initialization failed. Can't load '%s'", cfg_file_path);
        return false;
    }

    // Logging
    mdv_service_configure_logging();

    // Serializatior allocator
    mdv_binn_set_allocator();

    // DB metainformation storage
    svc->storage.metainf = mdv_metainf_storage_open(MDV_CONFIG.storage.path.ptr);
    if (!svc->storage.metainf)
    {
        MDV_LOGE("Service initialization failed. Can't create metainf storage '%s'", MDV_CONFIG.storage.path.ptr);
        return false;
    }

    if (!mdv_metainf_load(&svc->metainf, svc->storage.metainf))
    {
        MDV_LOGE("DB meta information loading failed. Path: '%s'", MDV_CONFIG.storage.path.ptr);
        return false;
    }

    mdv_metainf_validate(&svc->metainf);
    mdv_metainf_flush(&svc->metainf, svc->storage.metainf);

    if (mdv_nodes_load(&svc->storage.nodes, svc->storage.metainf) != MDV_OK)
    {
        MDV_LOGE("Nodes loading failed");
        return false;
    }

    // Tablespace
    if (mdv_tablespace_open(&svc->storage.tablespace, MDV_NODES_NUM) != MDV_OK
        && mdv_tablespace_create(&svc->storage.tablespace, MDV_NODES_NUM) != MDV_OK)
    {
        MDV_LOGE("DB tables space creation failed. Path: '%s'", MDV_CONFIG.storage.path.ptr);
        return false;
    }

    // Server
    svc->server = mdv_server_create(&svc->storage.tablespace, &svc->storage.nodes, &svc->metainf.uuid.value);

    if (!svc->server)
    {
        MDV_LOGE("Listener starting failed");
        return false;
    }

    // Print node information to log
    MDV_LOGI("Storage version: %u", svc->metainf.version.value);

    MDV_LOGI("Node UUID: %s", mdv_uuid_to_str(&svc->metainf.uuid.value).ptr);

    svc->is_started = false;

    return true;
}


void mdv_service_free(mdv_service *svc)
{
    mdv_server_free(svc->server);
    mdv_nodes_free(&svc->storage.nodes);
    mdv_storage_release(svc->storage.metainf);
    mdv_tablespace_close(&svc->storage.tablespace);
}


bool mdv_service_start(mdv_service *svc)
{
    svc->is_started = mdv_server_start(svc->server);

    while(svc->is_started)
    {
        mdv_sleep(1000);
    }

    return true;
}


void mdv_service_stop(mdv_service *svc)
{
    svc->is_started = false;
}
