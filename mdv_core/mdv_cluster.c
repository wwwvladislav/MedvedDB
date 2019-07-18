#include "mdv_cluster.h"
#include "mdv_config.h"
#include <mdv_log.h>
#include "mdv_conctx.h"


static void * mdv_cluster_conctx_create(mdv_descriptor fd, mdv_string const *addr, void *userdata, uint32_t type, mdv_channel_dir dir)
{
    mdv_cluster *cluster = userdata;
    (void)addr;
    return mdv_conctx_create(cluster->tablespace, fd, &cluster->uuid, type, dir);
}


mdv_errno mdv_cluster_create(mdv_cluster *cluster, mdv_tablespace *tablespace, mdv_nodes *nodes, mdv_uuid const *uuid)
{
    cluster->uuid       = *uuid;
    cluster->tablespace = tablespace;

    mdv_tracker_create(&cluster->tracker, nodes);

    mdv_chaman_config const config =
    {
        .peer =
        {
            .keepidle          = MDV_CONFIG.connection.keep_idle,
            .keepcnt           = MDV_CONFIG.connection.keep_count,
            .keepintvl         = MDV_CONFIG.connection.keep_interval
        },
        .threadpool =
        {
            .size = MDV_CONFIG.server.workers,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .userdata = cluster,
        .channel =
        {
            .select = mdv_conctx_select,
            .create = mdv_cluster_conctx_create,
            .recv   = mdv_conctx_recv,
            .close  = mdv_conctx_close
        }
    };

    cluster->chaman = mdv_chaman_create(&config);

    if (!cluster->chaman)
    {
        MDV_LOGE("Chaman not created");
        return MDV_FAILED;
    }

    mdv_errno err = mdv_chaman_listen(cluster->chaman, MDV_CONFIG.server.listen);

    if (err != MDV_OK)
    {
        MDV_LOGE("Listener failed with error: %s (%d)", mdv_strerror(err), err);
        mdv_chaman_free(cluster->chaman);
        return err;
    }

    return err;
}


void mdv_cluster_free(mdv_cluster *cluster)
{
    if(cluster)
    {
        mdv_chaman_free(cluster->chaman);
        mdv_tracker_free(&cluster->tracker);
    }
}


void mdv_cluster_update(mdv_cluster *cluster)
{
    // TODO: Run cluster discovery and update node list

    for(uint32_t i = 0; i < MDV_CONFIG.cluster.size; ++i)
    {
        mdv_errno err = mdv_chaman_connect(cluster->chaman, mdv_str((char*)MDV_CONFIG.cluster.nodes[i]), MDV_CLI_PEER);

        if (err != MDV_OK)
            MDV_LOGE("Connection failed with error: %s (%d)", mdv_strerror(err), err);
    }
}
