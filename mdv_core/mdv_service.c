#include "mdv_service.h"
#include <mdv_log.h>
#include <mdv_threads.h>
#include <mdv_binn.h>
#include <mdv_messages.h>
#include "storage/mdv_nodes.h"
#include "mdv_user.h"
#include "mdv_peer.h"


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


static bool mdv_service_cluster_create(mdv_service *svc)
{
    mdv_conctx_config const conctx_configs[] =
    {
        { MDV_CLI_USER, sizeof(mdv_user), &mdv_user_init, &mdv_user_free },
        { MDV_CLI_PEER, sizeof(mdv_peer), &mdv_peer_init, &mdv_peer_free }
    };

    mdv_cluster_config const cluster_config =
    {
        .uuid = svc->metainf.uuid.value,
        .channel =
        {
            .keepidle  = MDV_CONFIG.connection.keep_idle,
            .keepcnt   = MDV_CONFIG.connection.keep_count,
            .keepintvl = MDV_CONFIG.connection.keep_interval
        },
        .threadpool =
        {
            .size = MDV_CONFIG.server.workers,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .conctx =
        {
            .userdata = &svc->storage.tablespace,
            .size     = sizeof conctx_configs / sizeof *conctx_configs,
            .configs  = conctx_configs
        },
        .handlers =
        {
            .userdata = svc->storage.metainf,
            .reg_node = (mdv_cluster_reg_node_handler)&mdv_nodes_store
        }
    };

    if (mdv_cluster_create(&svc->cluster, &cluster_config) != MDV_OK)
    {
        MDV_LOGE("Cluster manager creation failed");
        return false;
    }

    // Load cluster nodes
    if (mdv_nodes_load(svc->storage.metainf, &svc->cluster.tracker) != MDV_OK)
    {
        MDV_LOGE("Nodes loading failed");
        return false;
    }

    return true;
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

    // DB meta information storage
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

    // Tablespace
    if (mdv_tablespace_open(&svc->storage.tablespace, MDV_NODES_NUM) != MDV_OK
        && mdv_tablespace_create(&svc->storage.tablespace, MDV_NODES_NUM) != MDV_OK)
    {
        MDV_LOGE("DB tables space creation failed. Path: '%s'", MDV_CONFIG.storage.path.ptr);
        return false;
    }

    // Cluster manager
    if (!mdv_service_cluster_create(svc))
        return false;

    MDV_LOGI("Storage version: %u", svc->metainf.version.value);

    MDV_LOGI("Node UUID: %s", mdv_uuid_to_str(&svc->metainf.uuid.value).ptr);

    svc->is_started = false;

    return true;
}


void mdv_service_free(mdv_service *svc)
{
    mdv_cluster_free(&svc->cluster);
    mdv_storage_release(svc->storage.metainf);
    mdv_tablespace_close(&svc->storage.tablespace);
}


bool mdv_service_start(mdv_service *svc)
{
    svc->is_started = mdv_cluster_bind(&svc->cluster, MDV_CONFIG.server.listen) == MDV_OK;

    for(uint32_t i = 0; i < MDV_CONFIG.cluster.size; ++i)
    {
        mdv_cluster_connect(&svc->cluster, mdv_str((char*)MDV_CONFIG.cluster.nodes[i]), MDV_CLI_PEER);
    }

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
