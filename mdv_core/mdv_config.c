#include "mdv_config.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <ini.h>
#include <string.h>


mdv_config_t mdv_config;

mdv_mempool(cfg_mempool, 256);


static int mdv_cfg_handler(void* user, const char* section, const char* name, const char* value)
{
    #define MDV_CFG_MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MDV_CFG_MATCH("server", "listen"))
    {
        mdv_config.listen = mdv_str_pdup(&cfg_mempool, (char const *)value);
        MDV_LOGI("Listen: %s", mdv_config.listen.data);
        return 1;
    }
    else
    {
        MDV_LOGE("Unknown section/name: [%s] %s", section, name);
        return 0;
    }
    return 1;
}


bool mdv_load_config(char const *path)
{
    if (ini_parse(path, mdv_cfg_handler, &mdv_config) < 0)
    {
        MDV_LOGE("Can't load '%s'\n", path);
        return false;
    }

    return true;
}
