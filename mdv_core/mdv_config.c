#include "mdv_config.h"
#include <mdv_log.h>
#include <ini.h>
#include <string.h>
#include <stdlib.h>


mdv_config MDV_CONFIG;


static int mdv_cfg_handler(void* user, const char* section, const char* name, const char* value)
{
    mdv_config *config = (mdv_config *)user;

    #define MDV_CFG_MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    #define MDV_CFG_CHECK(v)                                                                    \
        if(mdv_str_empty(v))                                                                 \
        {                                                                                       \
            MDV_LOGI("No memory for configuration parameter %s:%s '%s'", section, name, value); \
            return 0;                                                                           \
        }

    if (MDV_CFG_MATCH("server", "listen"))
    {
        config->server.listen = mdv_str_pdup(config->mempool, value);
        MDV_CFG_CHECK(config->server.listen);
        MDV_LOGI("Listen: %s", config->server.listen.ptr);
        return 1;
    }
    else if (MDV_CFG_MATCH("storage", "path"))
    {
        config->storage.path = mdv_str_pdup(config->mempool, value);
        MDV_CFG_CHECK(config->storage.path);
        MDV_LOGI("Storage path: %s", config->storage.path.ptr);
        return 1;
    }
    else if (MDV_CFG_MATCH("log", "level"))
    {
        config->log.level = mdv_str_pdup(config->mempool, value);
        MDV_CFG_CHECK(config->log.level);
        MDV_LOGI("Log level: %s", config->log.level.ptr);
        return 1;
    }
    else if (MDV_CFG_MATCH("transaction", "batch_size"))
    {
        config->transaction.batch_size = atoi(value);
        MDV_LOGI("Transaction batch size: %u", config->transaction.batch_size);
        return 1;
    }
    else
    {
        MDV_LOGE("Unknown section/name: [%s] %s", section, name);
        return 0;
    }

    #undef MDV_CFG_MATCH
    #undef MDV_CFG_CHECK

    return 1;
}


static void mdv_set_config_defaults()
{
    MDV_CONFIG.log.level                = mdv_str_static("error");
    MDV_CONFIG.server.listen            = mdv_str_static("localhost:54222");
    MDV_CONFIG.storage.path             = mdv_str_static("./data");
    MDV_CONFIG.transaction.batch_size   = 1000;
}


bool mdv_load_config(char const *path)
{
    mdv_set_config_defaults();

    mdv_stack_clear(MDV_CONFIG.mempool);

    if (ini_parse(path, mdv_cfg_handler, &MDV_CONFIG) < 0)
    {
        MDV_LOGE("Can't load '%s'\n", path);
        return false;
    }

    if (mdv_str_empty(MDV_CONFIG.server.listen)
        || mdv_str_empty(MDV_CONFIG.storage.path))
    {
        MDV_LOGE("Mandatory configuration parameters weren't not provided\n");
        return false;
    }

    return true;
}
