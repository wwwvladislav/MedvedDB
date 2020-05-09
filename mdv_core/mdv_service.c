#include "mdv_service.h"
#include <mdv_log.h>
#include <mdv_threads.h>
#include <mdv_binn.h>
#include <mdv_alloc.h>
#include "mdv_config.h"
#include "mdv_core.h"


struct mdv_service
{
    mdv_core       *core;               ///< Core component for cluster nodes management and storage accessing.
    volatile bool   is_started;         ///< Flag indicates that the service is working
};


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


static mdv_service * mdv_service_create_impl()
{
    // Logging
    mdv_service_configure_logging();

    // Serializatior allocator
    mdv_binn_set_allocator();

    mdv_service *svc = mdv_alloc(sizeof(mdv_service),  "service");

    if(!svc)
    {
        MDV_LOGE("No memory for new service");
        return 0;
    }

    // Create core
    svc->core = mdv_core_create();

    if (!svc->core)
    {
        MDV_LOGE("Core creation failed");
        mdv_free(svc, "service");
        return 0;
    }

    svc->is_started = false;

    return svc;
}


mdv_service * mdv_service_create(char const *cfg_file_path)
{
    // Configuration
    if (!mdv_load_config(cfg_file_path))
    {
        MDV_LOGE("Service initialization failed. Can't load '%s'", cfg_file_path);
        return 0;
    }

    return mdv_service_create_impl();
}


mdv_service * mdv_service_create_with_config(char const *cfg)
{
    // Configuration
    if (!mdv_apply_config(cfg))
    {
        MDV_LOGE("Service initialization failed. Can't apply configuration");
        return 0;
    }

    return mdv_service_create_impl();
}


void mdv_service_free(mdv_service *svc)
{
    if(svc)
    {
        mdv_core_free(svc->core);
        mdv_free(svc, "service");
    }
}


bool mdv_service_start(mdv_service *svc)
{
    svc->is_started = mdv_core_listen(svc->core);

    mdv_core_connect(svc->core);

    return svc->is_started;
}


void mdv_service_wait(mdv_service *svc)
{
    while(svc->is_started)
    {
        mdv_sleep(1000);
    }
}


void mdv_service_stop(mdv_service *svc)
{
    svc->is_started = false;
}
