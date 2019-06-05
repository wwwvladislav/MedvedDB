#include "mdv_config.h"
#include <mdv_log.h>
#include <ini.h>
#include <string.h>


mdv_config_t mdv_config;


static int mdv_cfg_handler(void* user, const char* section, const char* name, const char* value)
{
    #define MDV_CFG_MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MDV_CFG_MATCH("server", "listen"))
    {
        MDV_LOGI("Listen: %s", value);
        mdv_string_t const str = mdv_str((char*)value); // discards const qualifier, but mdv_str2addr does not modify string.
        return mdv_str2addr(str, &mdv_config.listen);
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
