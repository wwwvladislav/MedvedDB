#include "mdv_service.h"
#include <mdv_log.h>
#include <mdv_threads.h>
#include <mdv_binn.h>
#include "mdv_config.h"


static void mdv_service_configure_logging()
{
    if (!!MDV_CONFIG.log.level)
    {
        switch(*MDV_CONFIG.log.level)
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


bool mdv_service_create(mdv_service *svc, char const *cfg_file_path)
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

    // Create core
    svc->core = mdv_core_create();

    if (!svc->core)
    {
        MDV_LOGE("Core creation failed");
        return false;
    }

    svc->is_started = false;

    return true;
}


void mdv_service_free(mdv_service *svc)
{
    mdv_core_free(svc->core);
}


bool mdv_service_start(mdv_service *svc)
{
    svc->is_started = mdv_core_listen(svc->core);

    mdv_core_connect(svc->core);

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
