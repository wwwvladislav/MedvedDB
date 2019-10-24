#include "mdv_peer.h"
#include "mdv_p2pmsg.h"
#include "mdv_config.h"
#include "mdv_conctx.h"
#include "event/mdv_types.h"
#include "event/mdv_node.h"
#include "event/mdv_link.h"
#include "event/mdv_topology.h"
#include <mdv_version.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_dispatcher.h>
#include <mdv_rollbacker.h>
#include <mdv_ctypes.h>
#include <stdatomic.h>


struct mdv_peer
{
    mdv_conctx              base;           ///< connection context base type
    atomic_uint             rc;             ///< References counter
    mdv_channel_dir         dir;            ///< Channel direction
    mdv_uuid                uuid;           ///< current node uuid
    mdv_uuid                peer_uuid;      ///< peer global uuid
    uint32_t                peer_id;        ///< peer local unique identifier
    mdv_dispatcher         *dispatcher;     ///< Messages dispatcher
    mdv_ebus               *ebus;           ///< Events bus
};


static mdv_errno mdv_peer_connected(mdv_peer *peer, char const *addr, mdv_uuid const *uuid, uint32_t *id);
static void      mdv_peer_disconnected(mdv_peer *peer);


static mdv_errno mdv_peer_reply(mdv_peer *peer, mdv_msg const *msg);


static mdv_errno mdv_peer_hello_reply(mdv_peer *peer, uint16_t id, mdv_msg_p2p_hello const *msg)
{
    binn hey;

    if (!mdv_binn_p2p_hello(msg, &hey))
        return MDV_FAILED;

    mdv_msg const message =
    {
        .hdr =
        {
            .id     = mdv_message_id(p2p_hello),
            .number = id,
            .size   = binn_size(&hey)
        },
        .payload = binn_ptr(&hey)
    };

    mdv_errno err = mdv_peer_reply(peer, &message);

    binn_free(&hey);

    return err;
}


static mdv_errno mdv_peer_topodiff_reply(mdv_peer *peer, uint16_t id, mdv_msg_p2p_topodiff const *msg)
{
    binn obj;

    if (!mdv_binn_p2p_topodiff(msg, &obj))
        return MDV_FAILED;

    mdv_msg const message =
    {
        .hdr =
        {
            .id     = mdv_message_id(p2p_topodiff),
            .number = id,
            .size   = binn_size(&obj)
        },
        .payload = binn_ptr(&obj)
    };

    mdv_errno err = mdv_peer_reply(peer, &message);

    binn_free(&obj);

    return err;
}


static mdv_errno mdv_peer_hello_handler(mdv_msg const *msg, void *arg)
{
    mdv_peer *peer = arg;

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_p2p_msg_name(msg->hdr.id));
        return MDV_FAILED;
    }

    mdv_msg_p2p_hello req;

    if (!mdv_unbinn_p2p_hello(&binn_msg, &req))
    {
        binn_free(&binn_msg);
        return MDV_FAILED;
    }

    MDV_LOGI("<<<<< %s '%s'", mdv_uuid_to_str(&req.uuid).ptr, mdv_p2p_msg_name(msg->hdr.id));

    if(req.version != MDV_VERSION)
    {
        MDV_LOGE("Invalid peer version");
        binn_free(&binn_msg);
        return MDV_FAILED;
    }

    peer->peer_uuid = req.uuid;

    mdv_errno err = MDV_OK;

    if(peer->dir == MDV_CHIN)
    {
        mdv_msg_p2p_hello const hello =
        {
            .version = MDV_VERSION,
            .uuid    = peer->uuid,
            .listen  = MDV_CONFIG.server.listen.ptr
        };

        err = mdv_peer_hello_reply(peer, msg->hdr.number, &hello);
    }

    if (err == MDV_OK)
        err = mdv_peer_connected(peer, req.listen, &req.uuid, &peer->peer_id);
    else
        MDV_LOGE("Peer reply failed with error '%s' (%d)", mdv_strerror(err), err);

    binn_free(&binn_msg);

    if (err != MDV_OK)
        MDV_LOGE("Peer registration failed with error '%s' (%d)", mdv_strerror(err), err);

    return err;
}


static mdv_errno mdv_peer_linkstate_handler(mdv_msg const *msg, void *arg)
{
    mdv_peer *peer = arg;

    MDV_LOGI("<<<<< %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(msg->hdr.id));

/* TODO
    mdv_errno err = mdv_gossip_linkstate_handler(core, msg);

    // Update routing table
    if (err == MDV_OK)
        mdv_datasync_update_routes(&core->datasync);

    return err;
*/
    return MDV_OK;
}


static void mdv_peer_toposync_node_add(mdv_peer *peer, mdv_uuid const *uuid, char const *addr)
{
/*
    mdv_core *core = peer->core;
    mdv_tracker *tracker = core->tracker;

    size_t const addr_len = strlen(addr);
    size_t const node_size = offsetof(mdv_node, addr) + addr_len + 1;

    if (addr_len > MDV_ADDR_LEN_MAX)
    {
        MDV_LOGE("Invalid address length: %zu", addr_len);
        return;
    }

    char buf[node_size];

    mdv_node *node = (mdv_node *)buf;

    memset(node, 0, sizeof *node);

    node->size      = node_size;
    node->uuid      = *uuid;
    node->id        = 0;
    node->connected = 0;

    memcpy(node->addr, addr, addr_len + 1);

    // Save peer information and connection state in memory
    if (mdv_tracker_append(tracker, node, true))
        mdv_nodes_store_async(core->jobber, core->storage.metainf, node);
*/
}


static mdv_errno mdv_peer_toposync_handler(mdv_msg const *msg, void *arg)
{
/* TODO
    mdv_peer *peer = arg;
    mdv_core *core = peer->core;
    mdv_tracker *tracker = core->tracker;

    MDV_LOGI("<<<<< %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(msg->hdr.id));

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_p2p_msg_name(msg->hdr.id));
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &binn_msg);

    mdv_msg_p2p_toposync req;

    if (!mdv_unbinn_p2p_toposync(&binn_msg, &req))
    {
        MDV_LOGE("Topology synchronization request processing failed");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_p2p_toposync_free, &req);

    mdv_topology *topology = mdv_tracker_topology(tracker);

    if (!topology)
    {
        MDV_LOGE("Topology synchronization request processing failed");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_topology_release, topology);

    // Topologies difference calculation
    mdv_topology_delta *delta = mdv_topology_diff(topology, req.topology);

    if (!delta)
    {
        MDV_LOGE("Topology synchronization request processing failed");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_topology_delta_free, delta);

    mdv_msg_p2p_topodiff const topodiff =
    {
        .topology = delta->ab
    };

    mdv_errno err = mdv_peer_topodiff_reply(peer, msg->hdr.number, &topodiff);

    mdv_vector *ba_links = mdv_topology_links(delta->ba);

    if (delta->ba && !mdv_vector_empty(ba_links))
    {
        // Two isolated segments joined
        // TODO: Broadcast delta->ba to own segment.

        mdv_vector *ba_nodes = mdv_topology_nodes(delta->ba);

        mdv_vector_foreach(ba_nodes, mdv_toponode, node)
        {
            mdv_peer_toposync_node_add(peer, &node->uuid, node->addr);
        }

        mdv_vector_foreach(ba_links, mdv_topolink, link)
        {
            mdv_toponode const *src_node = mdv_vector_at(ba_nodes, link->node[0]);
            mdv_toponode const *dst_node = mdv_vector_at(ba_nodes, link->node[1]);
            mdv_tracker_linkstate(tracker, &src_node->uuid, &dst_node->uuid, true);
        }

        mdv_vector_release(ba_nodes);
    }

    mdv_vector_release(ba_links);

    mdv_rollback(rollbacker);

    return err;
*/
    return MDV_FAILED;
}


static mdv_errno mdv_peer_topodiff_handler(mdv_msg const *msg, void *arg)
{
/* TODO
    mdv_peer *peer = arg;
    mdv_core *core = peer->core;
    mdv_tracker *tracker = core->tracker;

    MDV_LOGI("<<<<< %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(msg->hdr.id));

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_p2p_msg_name(msg->hdr.id));
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &binn_msg);

    mdv_msg_p2p_topodiff req;

    if (!mdv_unbinn_p2p_topodiff(&binn_msg, &req))
    {
        MDV_LOGE("Topology difference processing failed");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_p2p_topodiff_free, &req);

    mdv_topology *topology = req.topology;

    if (topology)
    {
        // TODO: Broadcast topology to own segment if segment was isolated.

        mdv_vector *toponodes = mdv_topology_nodes(topology);
        mdv_vector *topolinks = mdv_topology_links(topology);

        mdv_vector_foreach(toponodes, mdv_toponode, node)
        {
            mdv_peer_toposync_node_add(peer, &node->uuid, node->addr);
        }

        mdv_vector_foreach(topolinks, mdv_topolink, link)
        {
            mdv_toponode const *src_node = mdv_vector_at(toponodes, link->node[0]);
            mdv_toponode const *dst_node = mdv_vector_at(toponodes, link->node[1]);
            mdv_tracker_linkstate(tracker, &src_node->uuid, &dst_node->uuid, true);
        }

        mdv_vector_release(toponodes);
        mdv_vector_release(topolinks);
    }

    mdv_rollback(rollbacker);

    // Update routing table
    mdv_datasync_update_routes(&core->datasync);
*/
    return MDV_OK;
}



static mdv_errno mdv_peer_cfslog_sync_handler(mdv_msg const *msg, void *arg)
{
/* TODO
    mdv_peer *peer = arg;
    mdv_core *core = peer->core;

    MDV_LOGI("<<<<< %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(msg->hdr.id));

    return mdv_datasync_cfslog_sync_handler(&core->datasync, peer->peer_id, msg);
*/
    return MDV_OK;
}


static mdv_errno mdv_peer_cfslog_state_handler(mdv_msg const *msg, void *arg)
{
/* TODO
    mdv_peer *peer = arg;
    mdv_core *core = peer->core;

    MDV_LOGI("<<<<< %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(msg->hdr.id));

    return mdv_datasync_cfslog_state_handler(&core->datasync, peer->peer_id, msg);
*/
    return MDV_OK;
}


static mdv_errno mdv_peer_cfslog_data_handler(mdv_msg const *msg, void *arg)
{
/* TODO
    mdv_peer *peer = arg;
    mdv_core *core = peer->core;

    MDV_LOGI("<<<<< %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(msg->hdr.id));

    mdv_errno err = mdv_datasync_cfslog_data_handler(&core->datasync, peer->peer_id, msg);

    if (err == MDV_OK)
        mdv_committer_start(&core->committer);

    return err;
*/
    return MDV_OK;
}


/**
 * @brief Post hello message
 */
static mdv_errno mdv_peer_hello(mdv_peer *peer)
{
    mdv_msg_p2p_hello hello =
    {
        .version = MDV_VERSION,
        .uuid    = peer->uuid,
        .listen  = MDV_CONFIG.server.listen.ptr
    };

    binn hey;

    if (!mdv_binn_p2p_hello(&hello, &hey))
        return MDV_FAILED;

    mdv_msg message =
    {
        .hdr =
        {
            .id = mdv_message_id(p2p_hello),
            .size = binn_size(&hey)
        },
        .payload = binn_ptr(&hey)
    };

    mdv_errno err = mdv_peer_post(peer, &message);

    binn_free(&hey);

    return err;
}


/**
 * @brief Post topology synchronization message
 */
static mdv_errno mdv_peer_toposync(mdv_peer *peer)
{
/* TODO
    mdv_core *core = peer->core;
    mdv_tracker *tracker = core->tracker;

    mdv_topology *topology = mdv_tracker_topology(tracker);

    if (!topology)
    {
        MDV_LOGE("Topology synchronization request failed");
        return MDV_FAILED;
    }

    mdv_msg_p2p_toposync const toposync =
    {
        .topology = topology
    };

    binn obj;

    if (!mdv_binn_p2p_toposync(&toposync, &obj))
    {
        MDV_LOGE("Topology synchronization request failed");
        mdv_topology_release(topology);
        return MDV_FAILED;
    }

    mdv_msg message =
    {
        .hdr =
        {
            .id = mdv_message_id(p2p_toposync),
            .size = binn_size(&obj)
        },
        .payload = binn_ptr(&obj)
    };

    mdv_errno err = mdv_peer_post(peer, &message);

    binn_free(&obj);

    mdv_topology_release(topology);

    return err;
*/
    return MDV_OK;
}


static mdv_errno mdv_peer_evt_link_state_broadcast(void *arg, mdv_event *event)
{
    mdv_peer *peer = arg;
    mdv_evt_link_state_broadcast *link_state = (mdv_evt_link_state_broadcast *)event;

    // TODO

    return MDV_NO_IMPL;
}


static mdv_errno mdv_peer_evt_topology_sync(void *arg, mdv_event *event)
{
    mdv_peer *peer = arg;
    mdv_evt_topology_sync *toposync = (mdv_evt_topology_sync *)event;

    // TODO

    return MDV_NO_IMPL;
}


static const mdv_event_handler_type mdv_peer_handlers[] =
{
    { MDV_EVT_LINK_STATE_BROADCAST, mdv_peer_evt_link_state_broadcast },
    { MDV_EVT_TOPOLOGY_SYNC, mdv_peer_evt_topology_sync },
};


mdv_peer * mdv_peer_create(mdv_uuid const *uuid,
                           mdv_channel_dir dir,
                           mdv_descriptor fd,
                           mdv_ebus *ebus)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(3);

    mdv_peer *peer = mdv_alloc(sizeof(mdv_peer), "peerctx");

    if (!peer)
    {
        MDV_LOGE("No memory for peer connection context");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, peer, "peerctx");

    MDV_LOGD("Peer %p initialization", peer);

    static mdv_iconctx iconctx =
    {
        .retain = (mdv_conctx * (*)(mdv_conctx *)) mdv_peer_retain,
        .release = (uint32_t (*)(mdv_conctx *))    mdv_peer_release
    };

    atomic_init(&peer->rc, 1);

    peer->base.type = MDV_CTX_PEER;
    peer->base.vptr = &iconctx;

    peer->dir = dir;
    peer->uuid = *uuid;
    peer->peer_id = 0;

    peer->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, peer->ebus);

    peer->dispatcher = mdv_dispatcher_create(fd);

    if (!peer->dispatcher)
    {
        MDV_LOGE("Peer connection context creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_dispatcher_free, peer->dispatcher);

    mdv_dispatcher_handler const handlers[] =
    {
        { mdv_message_id(p2p_hello),        &mdv_peer_hello_handler,        peer },
        { mdv_message_id(p2p_linkstate),    &mdv_peer_linkstate_handler,    peer },
        { mdv_message_id(p2p_toposync),     &mdv_peer_toposync_handler,     peer },
        { mdv_message_id(p2p_topodiff),     &mdv_peer_topodiff_handler,     peer },
        { mdv_message_id(p2p_cfslog_sync),  &mdv_peer_cfslog_sync_handler,  peer },
        { mdv_message_id(p2p_cfslog_state), &mdv_peer_cfslog_state_handler, peer },
        { mdv_message_id(p2p_cfslog_data),  &mdv_peer_cfslog_data_handler,  peer },
    };

    for(size_t i = 0; i < sizeof handlers / sizeof *handlers; ++i)
    {
        if (mdv_dispatcher_reg(peer->dispatcher, handlers + i) != MDV_OK)
        {
            MDV_LOGE("Messages dispatcher handler not registered");
            mdv_rollback(rollbacker);
            return 0;
        }
    }

    if (mdv_ebus_subscribe_all(peer->ebus,
                               peer,
                               mdv_peer_handlers,
                               sizeof mdv_peer_handlers / sizeof *mdv_peer_handlers) != MDV_OK)
    {
        MDV_LOGE("Ebus subscription failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    if (dir == MDV_CHOUT)
    {
        if (mdv_peer_hello(peer) != MDV_OK)
        {
            MDV_LOGD("Peer handshake message failed");
            mdv_ebus_unsubscribe_all(peer->ebus,
                                     peer,
                                     mdv_peer_handlers,
                                     sizeof mdv_peer_handlers / sizeof *mdv_peer_handlers);
            mdv_rollback(rollbacker);
            return 0;
        }
    }

    MDV_LOGD("Peer %p initialized", peer);

    mdv_rollbacker_free(rollbacker);

    return peer;
}


static void mdv_peer_free(mdv_peer *peer)
{
    if(peer)
    {
        mdv_ebus_unsubscribe_all(peer->ebus,
                                 peer,
                                 mdv_peer_handlers,
                                 sizeof mdv_peer_handlers / sizeof *mdv_peer_handlers);
        mdv_peer_disconnected(peer);
        mdv_dispatcher_free(peer->dispatcher);
        mdv_ebus_release(peer->ebus);
        mdv_free(peer, "peerctx");
        MDV_LOGD("Peer %p freed", peer);
    }
}


mdv_peer * mdv_peer_retain(mdv_peer *peer)
{
    if (peer)
    {
        uint32_t rc = atomic_load_explicit(&peer->rc, memory_order_relaxed);

        if (!rc)
            return 0;

        while(!atomic_compare_exchange_weak(&peer->rc, &rc, rc + 1))
        {
            if (!rc)
                return 0;
        }
    }

    return peer;
}


uint32_t mdv_peer_release(mdv_peer *peer)
{
    if (peer)
    {
        uint32_t rc = atomic_fetch_sub_explicit(&peer->rc, 1, memory_order_relaxed) - 1;

        if (!rc)
            mdv_peer_free(peer);

        return rc;
    }

    return 0;
}


mdv_errno mdv_peer_send(mdv_peer *peer, mdv_msg *req, mdv_msg *resp, size_t timeout)
{
    MDV_LOGI(">>>>> %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(req->hdr.id));

    mdv_errno err = MDV_CLOSED;

    peer = mdv_peer_retain(peer);

    if (peer)
    {
        err = mdv_dispatcher_send(peer->dispatcher, req, resp, timeout);
        mdv_peer_release(peer);
    }

    return err;
}


mdv_errno mdv_peer_post(mdv_peer *peer, mdv_msg *msg)
{
    MDV_LOGI(">>>>> %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(msg->hdr.id));

    mdv_errno err = MDV_CLOSED;

    peer = mdv_peer_retain(peer);

    if (peer)
    {
        err = mdv_dispatcher_post(peer->dispatcher, msg);
        mdv_peer_release(peer);
    }

    return err;
}


static mdv_errno mdv_peer_reply(mdv_peer *peer, mdv_msg const *msg)
{
    MDV_LOGI(">>>>> %s '%s'",
             mdv_uuid_to_str(&peer->peer_uuid).ptr,
             mdv_p2p_msg_name(msg->hdr.id));
    return mdv_dispatcher_reply(peer->dispatcher, msg);
}


static mdv_errno mdv_peer_connected(mdv_peer *peer,
                                    char const *addr,
                                    mdv_uuid const *uuid,
                                    uint32_t *id)
{
    {
        mdv_evt_node_up *event = mdv_evt_node_up_create(uuid, addr);

        if (!event)
            return MDV_NO_MEM;

        mdv_errno err = mdv_ebus_publish(peer->ebus, &event->base, MDV_EVT_SYNC);

        if (err == MDV_OK)
            *id = event->node.id;

        mdv_evt_node_up_release(event);

        if (err != MDV_OK)
            return err;
    }

    {
        mdv_evt_link_state *event = peer->dir == MDV_CHIN
                ? mdv_evt_link_state_create(&peer->uuid, &peer->peer_uuid, &peer->uuid, true)
                : mdv_evt_link_state_create(&peer->uuid, &peer->uuid, &peer->peer_uuid, true);

        if (event)
        {
            mdv_ebus_publish(peer->ebus, &event->base, MDV_EVT_DEFAULT);
            mdv_evt_link_state_release(event);
        }
    }

    return MDV_OK;

/* TODO
    mdv_core *core = peer->core;
    mdv_tracker *tracker = core->tracker;

    size_t const addr_len = strlen(addr);
    size_t const node_size = offsetof(mdv_node, addr) + addr_len + 1;

    char buf[node_size];

    mdv_node *node = (mdv_node *)buf;

    memset(node, 0, sizeof *node);

    node->size      = node_size;
    node->uuid      = *uuid;
    node->id        = 0;
    node->connected = 1;

    memcpy(node->addr, addr, addr_len + 1);

    // Save peer information and connection state in memory
    mdv_errno err = mdv_tracker_peer_connected(tracker, node);

    // Update routing table
    if (err == MDV_OK)
    {
        mdv_datasync_update_routes(&core->datasync);
        mdv_datasync_start(&core->datasync);
    }

    if (peer->conctx->dir == MDV_CHOUT)
    {
        // Synchronize network topology
        mdv_peer_toposync(peer);

        // Link-state broadcasting to all network
        mdv_gossip_linkstate(core,
                            &core->metainf.uuid.value, MDV_CONFIG.server.listen.ptr,
                            &node->uuid, node->addr,
                            true);
    }
    else
    {
        // Save peer address in DB
        mdv_nodes_store_async(core->jobber, core->storage.metainf, node);
    }

    if (err == MDV_OK)
        *id = node->id;

    return err;
*/
}


static void mdv_peer_disconnected(mdv_peer *peer)
{
    mdv_evt_link_state *event = mdv_evt_link_state_create(&peer->uuid, &peer->uuid, &peer->peer_uuid, false);

    if (event)
    {
        mdv_ebus_publish(peer->ebus, &event->base, MDV_EVT_DEFAULT);
        mdv_evt_link_state_release(event);
    }
}


mdv_errno mdv_peer_recv(mdv_peer *peer)
{
    return mdv_dispatcher_read(peer->dispatcher);
}


mdv_descriptor mdv_peer_fd(mdv_peer *peer)
{
    return mdv_dispatcher_fd(peer->dispatcher);
}

