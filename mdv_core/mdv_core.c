#include "mdv_core.h"
#include "mdv_user.h"
#include "mdv_peer.h"
#include "mdv_config.h"
#include "mdv_p2pmsg.h"
#include "mdv_gossip.h"
#include "storage/mdv_nodes.h"
#include <mdv_log.h>
#include <mdv_messages.h>


enum { MDV_NODES_NUM = 256 };


static bool mdv_core_cluster_create(mdv_core *core)
{
    mdv_conctx_config const conctx_configs[] =
    {
        { MDV_CLI_USER, sizeof(mdv_user), &mdv_user_init, &mdv_user_free },
        { MDV_CLI_PEER, sizeof(mdv_peer), &mdv_peer_init, &mdv_peer_free }
    };

    mdv_cluster_config const cluster_config =
    {
        .uuid = core->metainf.uuid.value,
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
            .userdata = core,
            .size     = sizeof conctx_configs / sizeof *conctx_configs,
            .configs  = conctx_configs
        }
    };

    if (mdv_cluster_create(&core->cluster, &cluster_config) != MDV_OK)
    {
        MDV_LOGE("Cluster manager creation failed");
        return false;
    }

    // Load cluster nodes
    if (mdv_nodes_load(core->storage.metainf, &core->cluster.tracker) != MDV_OK)
    {
        MDV_LOGE("Nodes loading failed");
        return false;
    }

    return true;
}


bool mdv_core_create(mdv_core *core)
{
    // DB meta information storage
    core->storage.metainf = mdv_metainf_storage_open(MDV_CONFIG.storage.path.ptr);

    if (!core->storage.metainf)
    {
        MDV_LOGE("Service initialization failed. Can't create metainf storage '%s'", MDV_CONFIG.storage.path.ptr);
        return false;
    }

    if (!mdv_metainf_load(&core->metainf, core->storage.metainf))
    {
        MDV_LOGE("DB meta information loading failed. Path: '%s'", MDV_CONFIG.storage.path.ptr);
        return false;
    }

    mdv_metainf_validate(&core->metainf);
    mdv_metainf_flush(&core->metainf, core->storage.metainf);

    // Tablespace
    if (mdv_tablespace_open(&core->storage.tablespace, MDV_NODES_NUM) != MDV_OK
        && mdv_tablespace_create(&core->storage.tablespace, MDV_NODES_NUM) != MDV_OK)
    {
        MDV_LOGE("DB tables space creation failed. Path: '%s'", MDV_CONFIG.storage.path.ptr);
        return false;
    }

    // Cluster manager
    if (!mdv_core_cluster_create(core))
        return false;

    MDV_LOGI("Storage version: %u", core->metainf.version.value);

    MDV_LOGI("Node UUID: %s", mdv_uuid_to_str(&core->metainf.uuid.value).ptr);

    return true;
}


void mdv_core_free(mdv_core *core)
{
    mdv_cluster_free(&core->cluster);
    mdv_storage_release(core->storage.metainf);
    mdv_tablespace_close(&core->storage.tablespace);
}


bool mdv_core_listen(mdv_core *core)
{
    return mdv_cluster_bind(&core->cluster, MDV_CONFIG.server.listen) == MDV_OK;
}


void mdv_core_connect(mdv_core *core)
{
    for(uint32_t i = 0; i < MDV_CONFIG.cluster.size; ++i)
    {
        mdv_cluster_connect(&core->cluster, mdv_str((char*)MDV_CONFIG.cluster.nodes[i]), MDV_CLI_PEER);
    }
}


mdv_errno mdv_core_peer_connected(mdv_core *core, mdv_node *new_node)
{
    mdv_errno err = mdv_tracker_peer_connected(&core->cluster.tracker, new_node);
    mdv_nodes_store(core->storage.metainf, new_node);

    // Link state notification broadcast

    static _Thread_local mdv_gossip_peer tmp_arr[MDV_NODES_NUM];

    mdv_gossip_peers peers =
    {
        .size = sizeof tmp_arr / sizeof *tmp_arr,
        .peers = tmp_arr
    };

    mdv_gossip_peers_get(&core->cluster.tracker, &peers);

    for (uint32_t i = 0; i < peers.size; ++i)
    {
        MDV_LOGI("Peer broadcast: uid=%u, lid=%u", peers.peers[i].uid, peers.peers[i].lid);
    }

/*
    mdv_msg_p2p_linkstate linkstate =
    {
        .peer_1 = core->metainf.uuid.value,
        .peer_2 = new_node->uuid,
        .connected = 0,
        .peers_count = 0,
        .peers = 0
    };
*/

    return err;
}


void mdv_core_peer_disconnected(mdv_core *core, mdv_uuid const *uuid)
{
    mdv_tracker_peer_disconnected(&core->cluster.tracker, uuid);

    // TODO: Link state notification broadcast
}
