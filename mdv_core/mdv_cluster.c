#include "mdv_cluster.h"
#include "mdv_config.h"
#include "mdv_conctx.h"
#include "mdv_user.h"
#include "mdv_peer.h"
#include <mdv_log.h>
#include <string.h>


static mdv_errno mdv_cluster_reg_peer(void *userdata, char const *addr, mdv_uuid const *uuid, uint32_t *id)
{
    mdv_cluster *cluster = userdata;

    size_t const addr_len = strlen(addr);
    size_t const node_size = offsetof(mdv_node, addr) + addr_len + 1;

    char buf[node_size];

    mdv_node * node = (mdv_node *)buf;

    node->size   = node_size;
    node->uuid   = *uuid;
    node->id     = 0;
    node->active = 1;

    memcpy(node->addr, addr, addr_len + 1);

    mdv_errno err = mdv_tracker_reg(&cluster->tracker, node);

    if (err == MDV_OK)
        *id = node->id;

    return err;
}


static void mdv_cluster_unreg_peer(void *userdata, mdv_uuid const *uuid)
{
    mdv_cluster *cluster = userdata;
    mdv_tracker_unreg(&cluster->tracker, uuid);
}


/**
 * @brief Select connection context
 *
 * @param fd [in]       file descriptor
 * @param type [out]    connection context type
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
static mdv_errno mdv_cluster_conctx_select(mdv_descriptor fd, uint8_t *type)
{
    mdv_msg_tag tag;
    size_t len = sizeof tag;

    mdv_errno err = mdv_read(fd, &tag, &len);

    if (err == MDV_OK)
        *type = tag.tag;

    return err;
}


/**
 * @brief Create connection context
 *
 * @param config [in]   Connection context configuration
 * @param type [in]     Client type (MDV_CLI_USER or MDV_CLI_PEER)
 * @param dir [in]      Channel direction (MDV_CHIN or MDV_CHOUT)
 *
 * @return On success, return new connection context
 * @return On error, return NULL pointer
 */
static void * mdv_cluster_conctx_create(mdv_descriptor fd, mdv_string const *addr, void *userdata, uint8_t type, mdv_channel_dir dir)
{
    mdv_cluster *cluster = userdata;
    (void)addr;

    mdv_conctx_config const config =
    {
        .tablespace = cluster->tablespace,
        .fd         = fd,
        .uuid       = cluster->uuid,
        .userdata   = cluster,
        .handlers   =
        {
            .reg_peer   = mdv_cluster_reg_peer,
            .unreg_peer = mdv_cluster_unreg_peer
        }
    };

    switch(type)
    {
        case MDV_CLI_USER:
            return (mdv_conctx*)mdv_user_accept(&config);

        case MDV_CLI_PEER:
            return dir == MDV_CHIN
                        ? (mdv_conctx*)mdv_peer_accept(&config)
                        : (mdv_conctx*)mdv_peer_connect(&config);

        default:
            MDV_LOGE("Undefined client type: %u", type);
    }

    return 0;
}


/**
 * @brief Read incoming data
 *
 * @param conctx [in] connection context
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
static mdv_errno mdv_cluster_conctx_recv(void *ctx)
{
    mdv_conctx *conctx = ctx;

    switch(conctx->type)
    {
        case MDV_CLI_USER:
            return mdv_user_recv((mdv_user *)conctx);

        case MDV_CLI_PEER:
            return mdv_peer_recv((mdv_peer *)conctx);

        default:
            MDV_LOGE("Undefined client type: %u", conctx->type);
    }

    return MDV_FAILED;
}


/**
 * @brief Free allocated by connection context resources
 *
 * @param conctx [in] connection context
 */
static void mdv_cluster_conctx_closed(void *ctx)
{
    mdv_conctx *conctx = ctx;

    switch(conctx->type)
    {
        case MDV_CLI_USER:
            return mdv_user_free((mdv_user *)conctx);

        case MDV_CLI_PEER:
            return mdv_peer_free((mdv_peer *)conctx);

        default:
            MDV_LOGE("Undefined client type: %u", conctx->type);
    }
}


mdv_errno mdv_cluster_create(mdv_cluster *cluster, mdv_tablespace *tablespace, mdv_nodes *nodes, mdv_uuid const *uuid)
{
    cluster->uuid       = *uuid;
    cluster->tablespace = tablespace;

    mdv_tracker_create(&cluster->tracker, nodes);

    mdv_chaman_config const config =
    {
        .channel =
        {
            .keepidle   = MDV_CONFIG.connection.keep_idle,
            .keepcnt    = MDV_CONFIG.connection.keep_count,
            .keepintvl  = MDV_CONFIG.connection.keep_interval,
            .select     = mdv_cluster_conctx_select,
            .create     = mdv_cluster_conctx_create,
            .recv       = mdv_cluster_conctx_recv,
            .close      = mdv_cluster_conctx_closed

        },
        .threadpool =
        {
            .size = MDV_CONFIG.server.workers,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .userdata = cluster
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
