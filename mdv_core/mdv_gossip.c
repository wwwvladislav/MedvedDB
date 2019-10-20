#include "mdv_gossip.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <mdv_algorithm.h>
#include <mdv_rollbacker.h>
#include <mdv_hash.h>
#include <stdlib.h>
#include "mdv_peer.h"
#include "mdv_p2pmsg.h"
#include "mdv_config.h"
#include "mdv_tracker.h"


typedef struct mdv_gossip_peers_set
{
    uint32_t          capacity;     ///< Peers set capacity
    mdv_gossip_peers *data;         ///< Storage for peers
} mdv_gossip_peers_set;


static mdv_gossip_peers * mdv_gossip_peers_alloc(uint32_t capacity)
{
    mdv_gossip_peers *peers = mdv_alloc(offsetof(mdv_gossip_peers, peers) + sizeof(mdv_gossip_peer) * capacity, "gossip_peers");

    if (peers)
        peers->size = 0;
    else
        MDV_LOGE("No memory for gossip peers allocation");

    return peers;
}


static mdv_gossip_peers * mdv_gossip_peers_realloc(mdv_gossip_peers *ptr, uint32_t capacity)
{
    mdv_gossip_peers *peers = mdv_realloc(ptr, offsetof(mdv_gossip_peers, peers) + sizeof(mdv_gossip_peer) * capacity, "gossip_peers");

    if (!peers)
        MDV_LOGE("No memory for gossip peers reallocation");

    return peers;
}


static void mdv_gossip_peer_add(mdv_node *node, void *arg)
{
    if (node->id == MDV_LOCAL_ID)
        return;             // Skip current node

    mdv_gossip_peers_set *set = arg;

    if (set->data->size >= set->capacity)
    {
        // Most likely this reallocation isn't happen.
        mdv_gossip_peers *peers = mdv_gossip_peers_realloc(set->data, set->capacity + 16);

        if (peers)
        {
            set->capacity += 16;
            set->data = peers;
        }
    }

    if (set->data->size < set->capacity)
    {
        mdv_gossip_peer *peer = &set->data->peers[set->data->size++];
        peer->uid = mdv_hash_murmur2a(&node->uuid, sizeof(node->uuid), 0);
        peer->lid = node->id;
    }
}


static int mdv_gossip_peer_cmp(const void *p1, const void *p2)
{
    mdv_gossip_peer const *peer_1 = p1;
    mdv_gossip_peer const *peer_2 = p2;
    return (int)peer_1->uid - peer_2->uid;
}


static mdv_gossip_peers * mdv_gossip_peers_get(mdv_tracker *tracker, bool add_self)
{
/*
    size_t const peers_count = mdv_tracker_peers_count(tracker) + 16;

    mdv_gossip_peers_set peers_set =
    {
        .capacity = peers_count,
        .data = mdv_gossip_peers_alloc(peers_count)
    };

    if (peers_set.data)
    {
        peers_set.data->size = 0;

        if (add_self)
        {
            mdv_gossip_peer *peer = &peers_set.data->peers[peers_set.data->size++];
            peer->uid = mdv_hash_murmur2a(mdv_tracker_uuid(tracker), sizeof(mdv_uuid), 0);
            peer->lid = MDV_LOCAL_ID;
        }

        mdv_tracker_peers_foreach(tracker, &peers_set, &mdv_gossip_peer_add);
        qsort(peers_set.data->peers, peers_set.data->size, sizeof(*peers_set.data->peers), &mdv_gossip_peer_cmp);
    }

    return peers_set.data;
*/
    return 0;
}


static void mdv_gossip_peers_free(mdv_gossip_peers *peers)
{
    if (peers)
        mdv_free(peers, "gossip_peers");
}


static mdv_errno mdv_gossip_linkstate_post(mdv_tracker *tracker, mdv_gossip_peers *peers, mdv_msg_p2p_linkstate const *linkstate)
{
    binn obj;

    if (mdv_binn_p2p_linkstate(linkstate, &obj))
    {
        mdv_msg message =
        {
            .hdr =
            {
                .id = mdv_message_id(p2p_linkstate),
                .size = binn_size(&obj)
            },
            .payload = binn_ptr(&obj)
        };

        /* TODO
        for (uint32_t i = 0; i < peers->size; ++i)
            mdv_tracker_peers_call(tracker, peers->peers[i].lid, &message, &mdv_peer_node_post);
        */

        binn_free(&obj);
    }
    else
        return MDV_FAILED;

    return MDV_OK;
}


mdv_errno mdv_gossip_linkstate(mdv_core            *core,
                               mdv_uuid const      *src_peer,
                               char const          *src_listen,
                               mdv_uuid const      *dst_peer,
                               char const          *dst_listen,
                               bool                 connected)
{
/* TODO
    mdv_tracker *tracker = core->tracker;
    mdv_errno err = MDV_OK;

    mdv_tracker_linkstate(tracker, src_peer, dst_peer, connected);

    mdv_gossip_peers *peers = mdv_gossip_peers_get(tracker, true);

    if (peers)
    {
        if (peers->size)
        {
            mdv_gossip_id *ids = mdv_staligned_alloc(sizeof(uint32_t), peers->size * sizeof(uint32_t), "gossip ids");

            if (ids)
            {
                for (uint32_t i = 0; i < peers->size; ++i)
                    ids[i] = peers->peers[i].uid;

                mdv_msg_p2p_linkstate const linkstate =
                {
                    .src =
                    {
                        .uuid = *src_peer,
                        .addr = src_listen
                    },
                    .dst =
                    {
                        .uuid = *dst_peer,
                        .addr = dst_listen
                    },
                    .connected      = connected,
                    .peers_count    = peers->size,
                    .peers          = ids
                };

                err = mdv_gossip_linkstate_post(tracker, peers, &linkstate);

                mdv_stfree(ids, "gossip ids");
            }
            else
            {
                MDV_LOGE("No memory for peers identifiers");
                err = MDV_NO_MEM;
            }
        }

        mdv_gossip_peers_free(peers);
    }

    return err;
*/
    return MDV_FAILED;
}


static int mdv_gossip_ids_cmp(const void *a, const void *b)
{
    mdv_gossip_peer const *peer_id = a;
    mdv_gossip_id const *id = b;
    return (int)peer_id->uid - *id;
}


static void mdv_gossip_node_track(mdv_core *core, mdv_toponode const *toponode)
{
/* TODO
    mdv_tracker *tracker = core->tracker;

    size_t const addr_len = strlen(toponode->addr);
    size_t const node_size = offsetof(mdv_node, addr) + addr_len + 1;

    char buf[node_size];

    mdv_node *node = (mdv_node *)buf;

    memset(node, 0, sizeof *node);

    node->size      = node_size;
    node->uuid      = toponode->uuid;
    node->active    = 1;

    memcpy(node->addr, toponode->addr, addr_len + 1);

    mdv_errno err = mdv_tracker_peer_connected(tracker, node);

    if (err == MDV_OK)
        mdv_nodes_store_async(core->jobber, core->storage.metainf, node);
*/
}


static void mdv_gossip_linkstate_track(mdv_core           *core,
                                       mdv_toponode const *src,
                                       mdv_toponode const *dst,
                                       bool                connected)
{
/* TODO
    mdv_tracker *tracker = core->tracker;

    mdv_gossip_node_track(core, src);
    mdv_gossip_node_track(core, dst);

    mdv_tracker_linkstate(tracker, &src->uuid, &dst->uuid, connected);
*/
}


mdv_errno mdv_gossip_linkstate_handler(mdv_core *core, mdv_msg const *msg)
{
/*
    mdv_tracker *tracker = core->tracker;

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(8);

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_p2p_msg_name(msg->hdr.id));
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &binn_msg);

    mdv_toponode *src             = mdv_unbinn_p2p_linkstate_src(&binn_msg);
    mdv_toponode *dst             = mdv_unbinn_p2p_linkstate_dst(&binn_msg);
    bool         *connected       = mdv_unbinn_p2p_linkstate_connected(&binn_msg);
    uint32_t     *src_peers_count = mdv_unbinn_p2p_linkstate_peers_count(&binn_msg);

    mdv_errno err = MDV_FAILED;

    do
    {
        if (   !src
            || !dst
            || !connected
            || !src_peers_count)
            break;

        mdv_gossip_linkstate_track(core, src, dst, *connected);

        if (*src_peers_count > MDV_MAX_CLUSTER_SIZE)
        {
            MDV_LOGW("Source peers count is too large");
            break;
        }

        mdv_gossip_id *src_peers = mdv_staligned_alloc(sizeof(mdv_gossip_id), *src_peers_count * sizeof(mdv_gossip_id), "gossip src ids");

        if (!src_peers)
        {
            MDV_LOGE("No memory for gossip src identifiers");
            err = MDV_NO_MEM;
            break;
        }

        mdv_rollbacker_push(rollbacker, mdv_stfree, src_peers, "gossip src ids");

        // Deserialize peers list from gossip message
        if (!mdv_unbinn_p2p_linkstate_peers(&binn_msg, src_peers, *src_peers_count))
            break;

        // Get all connected peers list
        mdv_gossip_peers *peers = mdv_gossip_peers_get(tracker, false);

        if (!peers)
            break;

        mdv_rollbacker_push(rollbacker, mdv_gossip_peers_free, peers);

        if (!peers->size)
        {
            err = MDV_OK;
            break;
        }

        uint32_t const dst_peers_count = peers->size;
        mdv_gossip_id *dst_peers = mdv_staligned_alloc(sizeof(mdv_gossip_id), dst_peers_count * sizeof(mdv_gossip_id), "gossip dst ids");

        if (!dst_peers)
        {
            MDV_LOGE("No memory for gossip identifiers");
            err = MDV_NO_MEM;
            break;
        }

        mdv_rollbacker_push(rollbacker, mdv_stfree, dst_peers, "gossip ids");

        // Debug checks
        dst_peers[0] = peers->peers[0].uid;

        for (uint32_t i = 1; i < peers->size; ++i)
        {
            if (dst_peers[i - 1] == peers->peers[i].uid)
            {
                MDV_LOGE("Peers identifiers must be unique");
                // TODO: We should notifiy all peers with not unique identifiers.
            }
            dst_peers[i] = peers->peers[i].uid;
        }

        // Exclude peers which sere received the gossip message
        peers->size = mdv_exclude(peers->peers, sizeof *peers->peers, peers->size,
                                  src_peers, sizeof *src_peers, *src_peers_count,
                                  &mdv_gossip_ids_cmp);

        if (!peers->size)
        {
            err = MDV_OK;
            break;
        }

        uint32_t union_size = 0;
        mdv_gossip_id *union_ids = mdv_staligned_alloc(sizeof(mdv_gossip_id), (dst_peers_count + *src_peers_count) * sizeof(mdv_gossip_id), "gossip union ids");

        if (!union_ids)
        {
            err = MDV_NO_MEM;
            break;
        }

        mdv_rollbacker_push(rollbacker, mdv_stfree, union_ids, "gossip union ids");

        // Add peers to gossip receivers list
        mdv_union_u32(dst_peers, dst_peers_count,
                      src_peers, *src_peers_count,
                      union_ids, &union_size);

        mdv_msg_p2p_linkstate const linkstate =
        {
            .src            = *src,
            .dst            = *dst,
            .connected      = *connected,
            .peers_count    = union_size,
            .peers          = union_ids
        };

        err = mdv_gossip_linkstate_post(tracker, peers, &linkstate);

    } while(0);

    mdv_rollback(rollbacker);

    return err;
*/
    return MDV_FAILED;
}

