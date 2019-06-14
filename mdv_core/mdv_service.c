#include "mdv_service.h"
#include <mdv_log.h>
#include <mdv_thread.h>
#include <mdv_binn.h>


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

#include "storage/mdv_table.h"

bool mdv_service_init(mdv_service *svc, char const *cfg_file_path)
{
    if (!mdv_load_config(cfg_file_path))
    {
        MDV_LOGE("Service initialization failed. Can't load '%s'\n", cfg_file_path);
        return false;
    }

    mdv_service_configure_logging();

    mdv_binn_set_allocator();

    svc->storage.metainf = mdv_metainf_storage_open(MDV_CONFIG.storage.path.ptr);
    if (!svc->storage.metainf)
    {
        MDV_LOGE("Service initialization failed. Can't create metainf storage '%s'\n", MDV_CONFIG.storage.path.ptr);
        return false;
    }

    if (!mdv_metainf_load(&svc->metainf, svc->storage.metainf))
    {
        MDV_LOGE("DB meta information loading was failed. Path: '%s'\n", MDV_CONFIG.storage.path.ptr);
        return false;
    }

    mdv_metainf_validate(&svc->metainf);
    mdv_metainf_flush(&svc->metainf, svc->storage.metainf);

    // Print node information to log
    MDV_LOGI("Storage version: %u", svc->metainf.version.value);
    char tmp[33];
    mdv_string uuid_str = mdv_str_static(tmp);
    if (mdv_uuid_to_str(&svc->metainf.uuid.value, &uuid_str))
        MDV_LOGI("Node UUID: %s", uuid_str.ptr);

    svc->is_started = false;

    // DEBUG
    mdv_table(3) tbl =
    {
        .name = mdv_str_static("my_table"),
        .size = 3,
        .fields =
        {
            { MDV_FLD_TYPE_CHAR,   mdv_str_static("col1") },
            { MDV_FLD_TYPE_BOOL,   mdv_str_static("col2") },
            { MDV_FLD_TYPE_UINT64, mdv_str_static("col3") }
        }
    };

    mdv_table_storage s = mdv_table_create(svc->storage.metainf, (mdv_table_base const *)&tbl);
    if (mdv_table_storage_ok(s))
    {
        mdv_table_close(&s);
    }
    // DEBUG

    return true;
}


void mdv_service_free(mdv_service *svc)
{
    mdv_storage_release(svc->storage.metainf);
}


bool mdv_service_start(mdv_service *svc)
{
    svc->is_started = true;

    while(svc->is_started)
    {
        mdv_usleep(1000);
    }
    return true;
}


void mdv_service_stop(mdv_service *svc)
{
    svc->is_started = false;
}
