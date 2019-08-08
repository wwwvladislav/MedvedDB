#include "mdv_cluster.h"
#include "mdv_log.h"
#include "mdv_alloc.h"
#include "mdv_socket.h"
#include "mdv_time.h"


static size_t mdv_hash_u8(void const *p)            { return *(uint8_t*)p; }
static int mdv_cmp_u8(void const *a, void const *b) { return (int)*(uint8_t*)a - *(uint8_t*)b; }


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
    uint8_t tag;
    size_t len = sizeof tag;

    mdv_errno err = mdv_read(fd, &tag, &len);

    if (err == MDV_OK)
        *type = tag;
    else
        MDV_LOGE("Channel selection failed with error %d", err);

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
    (void)addr;

    mdv_cluster *cluster = userdata;

    mdv_conctx_config const *conctx_config = mdv_hashmap_find(cluster->conctx_cfgs, type);

    if (!conctx_config)
    {
        MDV_LOGE("Unknown connection type %u", type);
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    if (dir == MDV_CHOUT)
    {
        mdv_errno err = mdv_write_all(fd, &type, sizeof type);

        if (err != MDV_OK)
        {
            MDV_LOGE("Channel tag was not sent");
            mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
            return 0;
        }
    }

    size_t size = offsetof(mdv_conctx, dataspace) + conctx_config->size;

    mdv_conctx *conctx = mdv_alloc(size, "conctx");

    if (!conctx)
    {
        MDV_LOGE("No memory for new connection context");
        return 0;
    }

    conctx->type                = type;
    conctx->dir                 = dir;
    conctx->dispatcher          = mdv_dispatcher_create(fd);
    conctx->created_time        = mdv_gettime();
    conctx->cluster             = cluster;

    if (!conctx->dispatcher)
    {
        MDV_LOGE("Messages dispatcher not created");
        mdv_free(conctx, "conctx");
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    mdv_errno err = conctx_config->init(conctx->dataspace, conctx, cluster->userdata);

    if (err != MDV_OK)
    {
        MDV_LOGE("Connection context initialization failed with error %d", err);
        mdv_dispatcher_free(conctx->dispatcher);
        mdv_free(conctx, "conctx");
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    return conctx;
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

    mdv_errno err = mdv_dispatcher_read(conctx->dispatcher);

    switch(err)
    {
        case MDV_OK:
        case MDV_EAGAIN:
            break;

        default:
            mdv_socket_shutdown(mdv_dispatcher_fd(conctx->dispatcher), MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
    }

    return err;
}


/**
 * @brief Free allocated by connection context resources
 *
 * @param conctx [in] connection context
 */
static void mdv_cluster_conctx_closed(void *ctx)
{
    mdv_conctx *conctx = ctx;

    mdv_conctx_config const *conctx_config = mdv_hashmap_find(conctx->cluster->conctx_cfgs, conctx->type);

    if (conctx_config)
    {
        conctx_config->free(conctx->dataspace, conctx);
        mdv_dispatcher_free(conctx->dispatcher);
        mdv_free(conctx, "conctx");
    }
    else
        MDV_LOGE("Unknown connection type %u", conctx->type);
}


mdv_errno mdv_cluster_create(mdv_cluster *cluster, mdv_cluster_config const *cluster_config)
{
    cluster->uuid     = cluster_config->uuid;
    cluster->userdata = cluster_config->conctx.userdata;

    mdv_errno err = mdv_tracker_create(&cluster->tracker, &cluster->uuid);

    if (err != MDV_OK)
    {
        MDV_LOGE("Cluster manager creation failed with error %d", err);
        return err;
    }

    mdv_chaman_config const config =
    {
        .channel =
        {
            .keepidle   = cluster_config->channel.keepidle,
            .keepcnt    = cluster_config->channel.keepcnt,
            .keepintvl  = cluster_config->channel.keepintvl,
            .select     = mdv_cluster_conctx_select,
            .create     = mdv_cluster_conctx_create,
            .recv       = mdv_cluster_conctx_recv,
            .close      = mdv_cluster_conctx_closed
        },
        .threadpool =
        {
            .size = cluster_config->threadpool.size,
            .thread_attrs =
            {
                .stack_size = cluster_config->threadpool.thread_attrs.stack_size
            }
        },
        .userdata = cluster
    };

    if (!mdv_hashmap_init(cluster->conctx_cfgs, mdv_conctx_config, type, cluster_config->conctx.size, &mdv_hash_u8, &mdv_cmp_u8))
    {
        MDV_LOGE("Cluster manager creation failed.");
        mdv_tracker_free(&cluster->tracker);
        return MDV_FAILED;
    }

    for(size_t i = 0; i < cluster_config->conctx.size; ++i)
    {
        mdv_hashmap_insert(cluster->conctx_cfgs, cluster_config->conctx.configs[i]);
    }

    cluster->chaman = mdv_chaman_create(&config);

    if (!cluster->chaman)
    {
        MDV_LOGE("Cluster manager creation failed. Chaman not created.");
        mdv_tracker_free(&cluster->tracker);
        mdv_hashmap_free(cluster->conctx_cfgs);
        return MDV_FAILED;
    }

    return err;
}


void mdv_cluster_free(mdv_cluster *cluster)
{
    if(cluster)
    {
        mdv_chaman_free(cluster->chaman);
        mdv_tracker_free(&cluster->tracker);
        mdv_hashmap_free(cluster->conctx_cfgs);
    }
}


mdv_errno mdv_cluster_bind(mdv_cluster *cluster, mdv_string const addr)
{
    mdv_errno err = mdv_chaman_listen(cluster->chaman, addr);

    if (err != MDV_OK)
        MDV_LOGE("Listener failed with error: %s (%d)", mdv_strerror(err), err);

    return err;
}


mdv_errno mdv_cluster_connect(mdv_cluster *cluster, mdv_string const addr, uint8_t type)
{
    mdv_errno err = mdv_chaman_connect(cluster->chaman, addr, type);

    if (err != MDV_OK)
        MDV_LOGE("Connection failed with error: %s (%d)", mdv_strerror(err), err);

    return err;
}
