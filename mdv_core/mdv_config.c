#include "mdv_config.h"
#include <mdv_log.h>
#include <ini.h>
#include <string.h>
#include <stdlib.h>


mdv_config MDV_CONFIG;


static size_t mdv_str2size(char const *str)
{
    #if UINTPTR_MAX == 0xffffffff
        return strtoul(str, 0, 10);
    #elif UINTPTR_MAX == 0xffffffffffffffff
        return strtoull(str, 0, 10);
    #else
        #error Unknown architecture
    #endif
}


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
    }
    else if (MDV_CFG_MATCH("server", "workers"))
    {
        config->server.workers = atoi(value);
        MDV_LOGI("Server workers: %u", config->server.workers);
    }

    else if (MDV_CFG_MATCH("storage", "path"))
    {
        mdv_string const trlog = mdv_str_static("/trlog");
        mdv_string const rowdata = mdv_str_static("/rowdata");

        config->storage.path = mdv_str_pdup(config->mempool, value);
        config->storage.trlog = mdv_str_pdup(config->mempool, value);
        config->storage.trlog = mdv_str_pcat(config->mempool, config->storage.trlog, trlog);
        config->storage.rowdata = mdv_str_pdup(config->mempool, value);
        config->storage.rowdata = mdv_str_pcat(config->mempool, config->storage.rowdata, rowdata);

        MDV_CFG_CHECK(config->storage.path);
        MDV_CFG_CHECK(config->storage.trlog);
        MDV_CFG_CHECK(config->storage.rowdata);

        MDV_LOGI("Storage path: %s", config->storage.path.ptr);
    }
    else if (MDV_CFG_MATCH("storage", "max_size"))
    {
        config->storage.max_size = mdv_str2size(value);
        MDV_LOGI("Storage maximum size: %zu", config->storage.max_size);
    }

    else if (MDV_CFG_MATCH("ebus", "workers"))
    {
        config->ebus.workers = atoi(value);
        MDV_LOGI("Ebus workers: %u", config->ebus.workers);
    }
    else if (MDV_CFG_MATCH("ebus", "queues"))
    {
        config->ebus.queues = atoi(value);
        MDV_LOGI("Ebus queues: %u", config->ebus.queues);
    }

    else if (MDV_CFG_MATCH("committer", "workers"))
    {
        config->committer.workers = atoi(value);
        MDV_LOGI("Committer workers: %u", config->committer.workers);
    }
    else if (MDV_CFG_MATCH("committer", "queues"))
    {
        config->committer.queues = atoi(value);
        MDV_LOGI("Committer queues: %u", config->committer.queues);
    }
    else if (MDV_CFG_MATCH("committer", "batch_size"))
    {
        config->committer.batch_size = atoi(value);
        MDV_LOGI("Committer batch size: %u", config->committer.batch_size);
    }

    else if (MDV_CFG_MATCH("log", "level"))
    {
        config->log.level = mdv_str_pdup(config->mempool, value);
        MDV_CFG_CHECK(config->log.level);
        MDV_LOGI("Log level: %s", config->log.level.ptr);
    }

    else if (MDV_CFG_MATCH("datasync", "workers"))
    {
        config->datasync.workers = atoi(value);
        MDV_LOGI("Datasync workers: %u", config->datasync.workers);
    }
    else if (MDV_CFG_MATCH("datasync", "queues"))
    {
        config->datasync.queues = atoi(value);
        MDV_LOGI("Datasync queues: %u", config->datasync.queues);
    }
    else if (MDV_CFG_MATCH("datasync", "batch_size"))
    {
        config->datasync.batch_size = atoi(value);
        MDV_LOGI("Datasync batch size: %u", config->datasync.batch_size);
    }

    else if (MDV_CFG_MATCH("fetcher", "workers"))
    {
        config->fetcher.workers = atoi(value);
        MDV_LOGI("Fetcher workers: %u", config->fetcher.workers);
    }
    else if (MDV_CFG_MATCH("fetcher", "queues"))
    {
        config->fetcher.queues = atoi(value);
        MDV_LOGI("Fetcher queues: %u", config->fetcher.queues);
    }
    else if (MDV_CFG_MATCH("fetcher", "batch_size"))
    {
        config->fetcher.batch_size = atoi(value);
        MDV_LOGI("Fetcher batch size: %u", config->fetcher.batch_size);
    }
    else if (MDV_CFG_MATCH("fetcher", "vm_stack"))
    {
        config->fetcher.vm_stack = atoi(value);
        MDV_LOGI("Fetcher VM stack: %u", config->fetcher.vm_stack);
    }
    else if (MDV_CFG_MATCH("fetcher", "views_lifetime"))
    {
        config->fetcher.views_lifetime = atoi(value);
        MDV_LOGI("Fetcher inactive views lifetime: %u seconds", config->fetcher.views_lifetime);
    }

    else if (MDV_CFG_MATCH("cluster", "node"))
    {
        mdv_string node = mdv_str_pdup(config->mempool, value);
        MDV_CFG_CHECK(node);
        if (config->cluster.size >= MDV_MAX_CLUSTER_SIZE)
        {
            MDV_LOGI("No space for node '%s'", value);
            return 0;
        }
        config->cluster.nodes[config->cluster.size++] = node.ptr;
        MDV_LOGI("Cluster node: %s", node.ptr);
    }

    else if (MDV_CFG_MATCH("connection", "retry_interval"))
    {
        MDV_CONFIG.connection.retry_interval = atoi(value);
        MDV_LOGI("Interval between reconnections: %u", MDV_CONFIG.connection.retry_interval);
    }
    else if (MDV_CFG_MATCH("connection", "keep_idle"))
    {
        MDV_CONFIG.connection.keep_idle = atoi(value);
        MDV_LOGI("Start keeplives after: %u seconds", MDV_CONFIG.connection.keep_idle);
    }
    else if (MDV_CFG_MATCH("connection", "keep_count"))
    {
        MDV_CONFIG.connection.keep_count = atoi(value);
        MDV_LOGI("Number of keepalives before death: %u", MDV_CONFIG.connection.keep_count);
    }
    else if (MDV_CFG_MATCH("connection", "keep_interval"))
    {
        MDV_CONFIG.connection.keep_interval = atoi(value);
        MDV_LOGI("Interval between keepalives: %u seconds", MDV_CONFIG.connection.keep_interval);
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
    MDV_CONFIG.log.level                    = mdv_str_static("error");

    MDV_CONFIG.server.listen                = mdv_str_static("tcp://localhost:54222");
    MDV_CONFIG.server.workers               = 8;

    MDV_CONFIG.connection.retry_interval    = 5;
    MDV_CONFIG.connection.keep_idle         = 5;
    MDV_CONFIG.connection.keep_count        = 10;
    MDV_CONFIG.connection.keep_interval     = 5;

    MDV_CONFIG.storage.path                 = mdv_str_static("./data");
    MDV_CONFIG.storage.trlog                = mdv_str_static("./data/trlog");
    MDV_CONFIG.storage.rowdata              = mdv_str_static("./data/rowdata");
    MDV_CONFIG.storage.max_size             = 4294967296u;

    MDV_CONFIG.ebus.workers                 = 4;
    MDV_CONFIG.ebus.queues                  = 4;

    MDV_CONFIG.committer.workers            = 4;
    MDV_CONFIG.committer.queues             = 4;
    MDV_CONFIG.committer.batch_size         = 32;

    MDV_CONFIG.datasync.workers             = 4;
    MDV_CONFIG.datasync.queues              = 4;
    MDV_CONFIG.datasync.batch_size          = 32;

    MDV_CONFIG.fetcher.workers              = 4;
    MDV_CONFIG.fetcher.queues               = 4;
    MDV_CONFIG.fetcher.batch_size           = 32;
    MDV_CONFIG.fetcher.vm_stack             = 64;
    MDV_CONFIG.fetcher.views_lifetime       = 30;

    MDV_CONFIG.cluster.size                 = 0;
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
