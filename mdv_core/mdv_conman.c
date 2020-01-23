#include "mdv_conman.h"
#include "mdv_user.h"
#include "mdv_peer.h"
#include "mdv_node.h"
#include "mdv_conctx.h"
#include "event/mdv_evt_types.h"
#include "event/mdv_evt_topology.h"
#include <mdv_chaman.h>
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <mdv_socket.h>
#include <mdv_ctypes.h>
#include <mdv_rollbacker.h>
#include <mdv_safeptr.h>
#include <mdv_bitset.h>


/// Connections manager
struct mdv_conman
{
    mdv_uuid         uuid;          ///< Current cluster node UUID
    mdv_chaman      *chaman;        ///< Channels manager
    mdv_ebus        *ebus;          ///< Events bus
    mdv_safeptr     *topology;      ///< Current network topology
};


/**
 * @brief Select connection context
 *
 * @param fd [in]       file descriptor
 * @param type [out]    connection context type
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
static mdv_errno mdv_conman_ctx_select(mdv_descriptor fd, uint8_t *type)
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
static void * mdv_conman_ctx_create(mdv_descriptor    fd,
                                    mdv_string const *addr,
                                    void             *userdata,
                                    uint8_t           type,
                                    mdv_channel_dir   dir)
{
    (void)addr;

    mdv_conman *conman = userdata;

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

    void *conctx = 0;

    switch(type)
    {
        case MDV_CTX_USER:
        {
            mdv_topology *topology = mdv_safeptr_get(conman->topology);
            conctx = mdv_user_create(&conman->uuid, fd, conman->ebus, topology);
            mdv_topology_release(topology);
            break;
        }

        case MDV_CTX_PEER:
            conctx = mdv_peer_create(&conman->uuid, dir, fd, conman->ebus);
            break;

        default:
            MDV_LOGE("Unknown connection context type");
            break;
    }

    if (!conctx)
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);

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
static mdv_errno mdv_conman_ctx_recv(void *userdata, void *ctx)
{
    (void)userdata;

    mdv_conctx *conctx = ctx;

    mdv_errno err = MDV_FAILED;
    mdv_descriptor fd = 0;

    switch(conctx->type)
    {
        case MDV_CTX_USER:
            fd = mdv_user_fd((mdv_user*)conctx);
            err = mdv_user_recv((mdv_user*)conctx);
            break;

        case MDV_CTX_PEER:
            fd = mdv_peer_fd((mdv_peer*)conctx);
            err = mdv_peer_recv((mdv_peer*)conctx);
            break;

        default:
            MDV_LOGE("Unknown connection context type");
            break;
    }

    switch(err)
    {
        case MDV_OK:
        case MDV_EAGAIN:
            break;

        default:
            mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
    }

    return err;
}


/**
 * @brief Free allocated by connection context resources
 *
 * @param conctx [in] connection context
 */
static void mdv_conman_ctx_closed(void *userdata, void *ctx)
{
    (void)userdata;
    mdv_conctx *conctx = ctx;
    conctx->vptr->release(conctx);
}


static void mdv_conman_connect_to_peers(mdv_conman *conman, mdv_topology *topology)
{
    mdv_vector *nodes = mdv_topology_nodes(topology);
    mdv_vector *links = mdv_topology_links(topology);

    mdv_bitset *linked_nodes = mdv_bitset_create(mdv_vector_size(nodes), &mdv_stallocator);

    if(linked_nodes)
    {
        mdv_vector_foreach(links, mdv_topolink, link)
        {
            mdv_toponode const *node[2] =
            {
                mdv_vector_at(nodes, link->node[0]),
                mdv_vector_at(nodes, link->node[1])
            };

            if (node[0]->id == MDV_LOCAL_ID)
                mdv_bitset_set(linked_nodes, link->node[1]);
            else if (node[1]->id == MDV_LOCAL_ID)
                mdv_bitset_set(linked_nodes, link->node[0]);
        }

        mdv_bitset_set(linked_nodes, MDV_LOCAL_ID);

        MDV_LOGE("SSSSSSSSSSSSSSSSS");

        for(size_t i = 0; i < mdv_bitset_size(linked_nodes); ++i)
        {
            if (!mdv_bitset_test(linked_nodes, i))
            {
                mdv_toponode const *node = mdv_vector_at(nodes, i);
                mdv_string const addr = mdv_str(node->addr);
                MDV_LOGE("OOOOOOOOOOOOOOOOOOO: %u", (unsigned)i);
                mdv_conman_connect(conman, addr, MDV_CTX_PEER);
            }
        }

        mdv_bitset_release(linked_nodes);
    }

    mdv_vector_release(nodes);
    mdv_vector_release(links);
}


static mdv_errno mdv_conman_evt_topology(void *arg, mdv_event *event)
{
    mdv_conman *conman = arg;
    mdv_evt_topology *topo = (mdv_evt_topology *)event;
    mdv_errno err = mdv_safeptr_set(conman->topology, topo->topology);
    // mdv_conman_connect_to_peers(conman, topo->topology);
    return err;
}


static const mdv_event_handler_type mdv_conman_handlers[] =
{
    { MDV_EVT_TOPOLOGY,    mdv_conman_evt_topology },
};


mdv_conman * mdv_conman_create(mdv_conman_config const *conman_config, mdv_ebus *ebus)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    mdv_conman *conman = mdv_alloc(sizeof(mdv_conman), "conman");

    if (!conman)
    {
        MDV_LOGE("No memory for connections manager");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, conman, "conman");

    conman->uuid     = conman_config->uuid;

    mdv_chaman_config const config =
    {
        .channel =
        {
            .retry_interval = conman_config->channel.retry_interval,
            .keepidle       = conman_config->channel.keepidle,
            .keepcnt        = conman_config->channel.keepcnt,
            .keepintvl      = conman_config->channel.keepintvl,
            .select         = mdv_conman_ctx_select,
            .create         = mdv_conman_ctx_create,
            .recv           = mdv_conman_ctx_recv,
            .close          = mdv_conman_ctx_closed
        },
        .threadpool =
        {
            .size = conman_config->threadpool.size,
            .thread_attrs =
            {
                .stack_size = conman_config->threadpool.thread_attrs.stack_size
            }
        },
        .userdata = conman
    };

    conman->chaman = mdv_chaman_create(&config);

    if (!conman->chaman)
    {
        MDV_LOGE("Cluster manager creation failed. Chaman not created.");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_chaman_free, conman->chaman);

    conman->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, ebus);

    conman->topology = mdv_safeptr_create(&mdv_empty_topology,
                                          (mdv_safeptr_retain_fn)mdv_topology_retain,
                                          (mdv_safeptr_release_fn)mdv_topology_release);

    if (!conman->topology)
    {
        MDV_LOGE("Safe pointer creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_safeptr_free, conman->topology);

    if (mdv_ebus_subscribe_all(conman->ebus,
                               conman,
                               mdv_conman_handlers,
                               sizeof mdv_conman_handlers / sizeof *mdv_conman_handlers) != MDV_OK)
    {
        MDV_LOGE("Ebus subscription failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_free(rollbacker);

    return conman;
}


void mdv_conman_free(mdv_conman *conman)
{
    if(conman)
    {
        mdv_chaman_free(conman->chaman);

        mdv_ebus_unsubscribe_all(conman->ebus,
                                 conman,
                                 mdv_conman_handlers,
                                 sizeof mdv_conman_handlers / sizeof *mdv_conman_handlers);

        mdv_safeptr_free(conman->topology);
        mdv_ebus_release(conman->ebus);

        mdv_free(conman, "conman");
    }
}


mdv_errno mdv_conman_bind(mdv_conman *conman, mdv_string const addr)
{
    mdv_errno err = mdv_chaman_listen(conman->chaman, addr);

    if (err != MDV_OK)
        MDV_LOGE("Listener failed with error: %s (%d)", mdv_strerror(err), err);

    return err;
}


mdv_errno mdv_conman_connect(mdv_conman *conman, mdv_string const addr, uint8_t type)
{
    mdv_errno err = mdv_chaman_dial(conman->chaman, addr, type);

    if (err != MDV_OK)
        MDV_LOGE("Connection failed with error: %s (%d)", mdv_strerror(err), err);

    return err;
}
