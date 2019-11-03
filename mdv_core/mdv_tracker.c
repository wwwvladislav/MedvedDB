#include "mdv_tracker.h"
#include "mdv_p2pmsg.h"
#include "storage/mdv_nodes.h"
#include "event/mdv_types.h"
#include "event/mdv_link.h"
#include "event/mdv_topology.h"
#include "event/mdv_routes.h"
#include "event/mdv_broadcast.h"
#include "mdv_config.h"
#include <mdv_limits.h>
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <mdv_algorithm.h>
#include <mdv_hashmap.h>
#include <mdv_mutex.h>
#include <mdv_router.h>
#include <string.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <assert.h>


/// Node identifier
typedef struct
{
    uint32_t   id;      ///< Unique identifier inside current server
    mdv_node  *node;    ///< Cluster node information
} mdv_node_id;


/// Nodes and network topology tracker
struct mdv_tracker
{
    atomic_uint_fast32_t    rc;             ///< References counter
    mdv_uuid                uuid;           ///< Global unique identifier for current node (i.e. self UUID)
    atomic_uint_fast32_t    max_id;         ///< Maximum node identifier
    mdv_storage            *storage;        ///< Nodes storage
    mdv_ebus               *ebus;           ///< Events bus

    mdv_mutex               nodes_mutex;    ///< nodes guard mutex
    mdv_hashmap            *nodes;          ///< Nodes map (UUID -> mdv_node)
    mdv_hashmap            *ids;            ///< Node identifiers (id -> mdv_node *)

    mdv_mutex               links_mutex;    ///< Links guard mutex
    mdv_hashmap            *links;          ///< Links (id -> id's vector)

    mdv_mutex               routes_mutex;   ///< Routes guard mutex
    mdv_vector             *routes;         ///< Routes
};


static mdv_node * mdv_tracker_insert(mdv_tracker *tracker, mdv_node const *node);
static mdv_node * mdv_tracker_find(mdv_tracker *tracker, mdv_uuid const *uuid);
static void       mdv_tracker_erase(mdv_tracker *tracker, mdv_node const *node);


/**
 * @brief Register node connection
 *
 * @param tracker [in]          Topology tracker
 * @param uuid [in]             node UUID
 * @param addr [in]             node address
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero error code
 */
static mdv_errno mdv_tracker_register(mdv_tracker *tracker, mdv_uuid const *uuid, char const *addr);


/**
 * @brief Extract network topology from tracker
 * @details In result topology links are sorted in ascending order.
 *
 * @param tracker [in]          Topology tracker
 *
 * @return On success return non NULL pointer to a network topology.
 */
static mdv_topology * mdv_tracker_topology(mdv_tracker *tracker);


/**
 * @brief Link state tracking between two peers.
 *
 * @param tracker [in]          Topology tracker
 * @param peer_1 [in]           First peer UUID
 * @param peer_2 [in]           Second peer UUID
 * @param connected [in]        Connection status
 *
 * @return On success, return MDV_OK
 * @return On error, return nonzero error code
 */
static mdv_errno mdv_tracker_linkstate(mdv_tracker     *tracker,
                                       mdv_uuid const  *peer_1,
                                       mdv_uuid const  *peer_2,
                                       bool             connected);


static size_t mdv_u32_hash(uint32_t const *id)
{
    return *id;
}

static int mdv_u32_keys_cmp(uint32_t const *id1, uint32_t const *id2)
{
    return (int)*id1 - *id2;
}


static size_t mdv_tracker_link_hash(mdv_tracker_link const *link)
{
    uint32_t a, b;

    if (link->id[0] < link->id[1])
    {
        a = link->id[0];
        b = link->id[1];
    }
    else
    {
        a = link->id[1];
        b = link->id[0];
    }

    static size_t const FNV_offset_basis = 0xcbf29ce484222325;
    static size_t const FNV_prime = 0x100000001b3;

    size_t hash = FNV_offset_basis;

    hash = (hash * FNV_prime) ^ a;
    hash = (hash * FNV_prime) ^ b;

    return hash;
}


static int mdv_tracker_link_cmp(mdv_tracker_link const *link1, mdv_tracker_link const *link2)
{
    uint32_t a1, b1, a2, b2;

    if (link1->id[0] < link1->id[1])
    {
        a1 = link1->id[0];
        b1 = link1->id[1];
    }
    else
    {
        a1 = link1->id[1];
        b1 = link1->id[0];
    }

    if (link2->id[0] < link2->id[1])
    {
        a2 = link2->id[0];
        b2 = link2->id[1];
    }
    else
    {
        a2 = link2->id[1];
        b2 = link2->id[0];
    }

    if (a1 < a2)        return -1;
    else if (a1 > a2)   return  1;
    else if (b1 < b2)   return -1;
    else if (b1 > b2)   return  1;

    return 0;
}


static mdv_node * mdv_tracker_insert(mdv_tracker *tracker, mdv_node const *node)
{
    mdv_node *entry = mdv_hashmap_insert(tracker->nodes, node, mdv_node_size(node));

    if (entry)
    {
        mdv_node_id const nid =
        {
            .id = node->id,
            .node = entry
        };

        if (mdv_hashmap_insert(tracker->ids, &nid, sizeof nid))
            return entry;

        mdv_hashmap_erase(tracker->nodes, &node->uuid);
        MDV_LOGW("Node '%s' discarded", mdv_uuid_to_str(&node->uuid).ptr);
    }
    else
        MDV_LOGW("Node '%s' discarded", mdv_uuid_to_str(&node->uuid).ptr);

    return 0;
}


static void mdv_tracker_erase(mdv_tracker *tracker, mdv_node const *node)
{
    mdv_hashmap_erase(tracker->ids, &node->id);
    mdv_hashmap_erase(tracker->nodes, &node->uuid);
}


static mdv_node * mdv_tracker_find(mdv_tracker *tracker, mdv_uuid const *uuid)
{
    return mdv_hashmap_find(tracker->nodes, uuid);
}


static uint32_t mdv_tracker_new_id(mdv_tracker *tracker)
{
    return atomic_fetch_add_explicit(&tracker->max_id, 1, memory_order_relaxed) + 1;
}


static void mdv_tracker_id_maximize(mdv_tracker *tracker, uint32_t id)
{
    for(uint64_t top_id = atomic_load(&tracker->max_id); id > top_id;)
    {
        if(atomic_compare_exchange_weak(&tracker->max_id, &top_id, id))
            break;
    }
}


static mdv_errno mdv_tracker_load(mdv_tracker *tracker)
{
    mdv_node const *node = mdv_nodes_current(&tracker->uuid, MDV_CONFIG.server.listen.ptr);

    mdv_tracker_id_maximize(tracker, node->id);

    if (!mdv_tracker_insert(tracker, node))
        return MDV_FAILED;

    return mdv_nodes_foreach(tracker->storage,
                             tracker,
                             (void (*)(void *, mdv_node const *))
                                    mdv_tracker_insert);
}


static mdv_errno mdv_tracker_evt_link_state(void *arg, mdv_event *event)
{
    mdv_tracker *tracker = arg;
    mdv_evt_link_state *link_state = (mdv_evt_link_state *)event;

    mdv_errno err;

    err = mdv_tracker_register(tracker, &link_state->src.uuid, link_state->src.addr);
    if (err != MDV_OK)
        return err;

    err = mdv_tracker_register(tracker, &link_state->dst.uuid, link_state->dst.addr);
    if (err != MDV_OK)
        return err;

    err = mdv_tracker_linkstate(
                    tracker,
                    &link_state->src.uuid,
                    &link_state->dst.uuid,
                    link_state->connected);

    if (err != MDV_OK)
    {
        MDV_LOGE("Link registration failed");
        return err;
    }

    mdv_topology *topology = mdv_tracker_topology(tracker);

    if (!topology)
    {
        MDV_LOGE("Topology calculation failed");
        return MDV_FAILED;
    }

    // Routes changed
    mdv_hashmap *routes = mdv_routes_find(topology, &tracker->uuid);

    if (routes)
    {
        mdv_evt_routes_changed *evt = mdv_evt_routes_changed_create(routes);

        if (evt)
        {
            if (mdv_ebus_publish(tracker->ebus, &evt->base, MDV_EVT_DEFAULT) != MDV_OK)
            {
                err = MDV_FAILED;
                MDV_LOGE("Routes notification failed");
            }

            mdv_evt_routes_changed_release(evt);
        }
        else
        {
            err = MDV_FAILED;
            MDV_LOGE("Routes notification failed");
        }
    }
    else
    {
        err = MDV_FAILED;
        MDV_LOGE("Routes calculation failed");
    }

    bool const is_current_node_connection_initiated =
        mdv_uuid_cmp(&tracker->uuid, &link_state->from) == 0
        && mdv_uuid_cmp(&tracker->uuid, &link_state->src.uuid) == 0;

    if (is_current_node_connection_initiated && topology)
    {
        if (link_state->connected)
        {
            // Topology synchronization
            mdv_evt_topology_sync *evt = mdv_evt_topology_sync_create(
                                                &tracker->uuid,
                                                &link_state->dst.uuid,
                                                topology);

            if (evt)
            {
                if (mdv_ebus_publish(tracker->ebus, &evt->base, MDV_EVT_DEFAULT) != MDV_OK)
                {
                    err = MDV_FAILED;
                    MDV_LOGE("Topology synchronization failed");
                }

                mdv_evt_topology_sync_release(evt);
            }
            else
            {
                err = MDV_FAILED;
                MDV_LOGE("Topology synchronization failed");
            }
        }
        else
        {
/*
            // Link state broadcasting
            mdv_evt_link_state_broadcast *evt = mdv_evt_link_state_broadcast_create(
                                                    routes,
                                                    &link_state->from,
                                                    &link_state->src,
                                                    &link_state->dst,
                                                    link_state->connected);

            if (evt)
            {
                if (mdv_ebus_publish(tracker->ebus, &evt->base, MDV_EVT_DEFAULT) != MDV_OK)
                {
                    err = MDV_FAILED;
                    MDV_LOGE("Link state broadcasting failed");
                }

                mdv_evt_link_state_broadcast_release(evt);
            }
            else
            {
                err = MDV_FAILED;
                MDV_LOGE("Link state broadcasting failed");
            }
*/
        }
    }

    mdv_hashmap_release(routes);
    mdv_topology_release(topology);

    return err;
}


static mdv_errno mdv_tracker_topology_broadcast(
                                mdv_tracker    *tracker,
                                mdv_uuid const *segment_gw,
                                bool            current_segment,
                                mdv_topology   *diff,
                                mdv_hashmap    *peers)
{
    mdv_hashmap *dst = _mdv_hashmap_create(
                                1,
                                0,
                                sizeof(mdv_uuid),
                                (mdv_hash_fn)mdv_uuid_hash,
                                (mdv_key_cmp_fn)mdv_uuid_cmp);

    if (!dst)
    {
        MDV_LOGE("Topology broadcasting request failed. No memory.");
        return MDV_NO_MEM;
    }

    if(current_segment)
    {
        mdv_hashmap_foreach(peers, mdv_uuid, uuid)
        {
            if (mdv_uuid_cmp(uuid, segment_gw) == 0)
            {
                if (!mdv_hashmap_insert(dst, uuid, sizeof *uuid))
                {
                    MDV_LOGE("Topology broadcasting request failed. No memory.");
                    mdv_hashmap_release(dst);
                    return MDV_NO_MEM;
                }
            }
        }
    }
    else
    {
        if (!mdv_hashmap_insert(dst, segment_gw, sizeof *segment_gw))
        {
            MDV_LOGE("Topology broadcasting request failed. No memory.");
            mdv_hashmap_release(dst);
            return MDV_NO_MEM;
        }
    }

    mdv_msg_p2p_topodiff const topodiff = { diff };

    binn obj;

    mdv_errno err = MDV_FAILED;

    if (mdv_binn_p2p_topodiff(&topodiff, &obj))
    {
        mdv_evt_broadcast_post *evt = mdv_evt_broadcast_post_create(
                                                        mdv_message_id(p2p_topodiff),
                                                        binn_size(&obj),
                                                        binn_ptr(&obj),
                                                        peers,
                                                        dst);

        if (evt)
        {
            err = mdv_ebus_publish(tracker->ebus, &evt->base, MDV_EVT_DEFAULT);
            mdv_evt_broadcast_post_release(evt);
        }
        else
        {
            err = MDV_NO_MEM;
            MDV_LOGE("No memory for broadcast message");
        }

        binn_free(&obj);
    }

    if (err != MDV_OK)
        MDV_LOGE("Topology broadcasting request failed");

    mdv_hashmap_release(dst);

    return err;
}


static mdv_errno mdv_tracker_evt_topology_sync(void *arg, mdv_event *event)
{
    mdv_tracker *tracker = arg;
    mdv_evt_topology_sync *evt = (mdv_evt_topology_sync *)event;

    if(mdv_uuid_cmp(&tracker->uuid, &evt->to) != 0)
        return MDV_OK;

    mdv_topology *topology = mdv_tracker_topology(tracker);

    if (!topology)
    {
        MDV_LOGE("Topology calculation failed");
        return MDV_FAILED;
    }

    mdv_hashmap *peers = mdv_topology_peers(topology, &tracker->uuid);

    if (!peers)
    {
        MDV_LOGE("No memory for peers map");
        mdv_topology_release(topology);
        return MDV_FAILED;
    }

    mdv_errno ret = MDV_OK;

    // Topologies difference synchronization
    {
        // Network segment of current node
        mdv_topology *diff = mdv_topology_diff(evt->topology, topology);

        if (diff && diff != &mdv_empty_topology)
        {
            mdv_errno err = mdv_tracker_topology_broadcast(tracker, &evt->from, true, diff, peers);

            if (err != MDV_OK)
                ret = err;

            mdv_topology_release(diff);
        }
    }

    {
        // Network segment of remote peer
        mdv_topology *diff = mdv_topology_diff(topology, evt->topology);

        if (diff && diff != &mdv_empty_topology)
        {
            mdv_errno err = mdv_tracker_topology_broadcast(tracker, &evt->from, false, diff, peers);

            if (err != MDV_OK)
                ret = err;

            mdv_topology_release(diff);
        }
    }

    mdv_hashmap_release(peers);
    mdv_topology_release(topology);

    return ret;
}


static mdv_errno mdv_tracker_evt_broadcast(void *arg, mdv_event *event)
{
    mdv_tracker *tracker = arg;
    mdv_evt_broadcast *evt = (mdv_evt_broadcast *)event;

    mdv_errno err = MDV_FAILED;

    switch(evt->msg_id)
    {
        case mdv_message_id(p2p_topodiff):
        {
            // TODO
            break;
        }

        default:
            MDV_LOGE("Unsupported event type was broadcasted");
            break;
    }

    mdv_topology *topology = mdv_tracker_topology(tracker);

    if (!topology)
    {
        MDV_LOGE("Topology calculation failed");
        return MDV_FAILED;
    }

    mdv_hashmap *peers = mdv_topology_peers(topology, &tracker->uuid);

    if (peers)
    {
        mdv_hashmap_foreach(evt->notified, mdv_uuid, entry)
            mdv_hashmap_erase(peers, entry);

        if (mdv_hashmap_size(peers))
        {
            mdv_hashmap_foreach(peers, mdv_uuid, entry)
            {
                if (!mdv_hashmap_insert(evt->notified, entry, sizeof *entry))
                {
                    MDV_LOGE("No memory for notified peers");
                    err = MDV_NO_MEM;
                    break;
                }
            }

            if (err == MDV_OK)
            {
                mdv_evt_broadcast_post *broadcast_post = mdv_evt_broadcast_post_create(
                                                evt->msg_id,
                                                evt->size,
                                                evt->data,
                                                evt->notified,
                                                peers);

                if (broadcast_post)
                {
                    err = mdv_ebus_publish(tracker->ebus, &broadcast_post->base, MDV_EVT_DEFAULT);
                    mdv_evt_broadcast_post_release(broadcast_post);
                }
                else
                {
                    err = MDV_NO_MEM;
                    MDV_LOGE("No memory for broadcast message");
                }
            }
        }

        mdv_hashmap_release(peers);
    }
    else
    {
        MDV_LOGE("Peers searching failed");
        err = MDV_FAILED;
    }

    mdv_topology_release(topology);

    return err;
}


static const mdv_event_handler_type mdv_tracker_handlers[] =
{
    { MDV_EVT_LINK_STATE,       mdv_tracker_evt_link_state },
    { MDV_EVT_TOPOLOGY_SYNC,    mdv_tracker_evt_topology_sync },
    { MDV_EVT_BROADCAST,        mdv_tracker_evt_broadcast },
};


mdv_tracker * mdv_tracker_create(mdv_uuid const *uuid,
                                 mdv_storage    *storage,
                                 mdv_ebus       *ebus)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(8);

    mdv_tracker *tracker = mdv_alloc(sizeof(mdv_tracker), "tracker");

    if (!tracker)
    {
        MDV_LOGE("No memory for tracker");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, tracker, "tracker");

    atomic_init(&tracker->rc, 1);
    atomic_init(&tracker->max_id, 0);

    tracker->uuid = *uuid;

    tracker->storage = mdv_storage_retain(storage);
    tracker->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_storage_release, tracker->storage);
    mdv_rollbacker_push(rollbacker, mdv_ebus_release, tracker->ebus);

    tracker->nodes = mdv_hashmap_create(mdv_node,
                                        uuid,
                                        256,
                                        mdv_uuid_hash,
                                        mdv_uuid_cmp);
    if (!tracker->nodes)
    {
        MDV_LOGE("There is no memory for nodes");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, tracker->nodes);


    if (mdv_mutex_create(&tracker->nodes_mutex) != MDV_OK)
    {
        MDV_LOGE("Nodes storage mutex not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &tracker->nodes_mutex);


    tracker->ids = mdv_hashmap_create(mdv_node_id,
                                      id,
                                      256,
                                      mdv_u32_hash,
                                      mdv_u32_keys_cmp);

    if (!tracker->ids)
    {
        MDV_LOGE("There is no memory for nodes");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, tracker->ids);


    tracker->links = mdv_hashmap_create(mdv_tracker_link,
                                        id,
                                        256,
                                        mdv_tracker_link_hash,
                                        mdv_tracker_link_cmp);

    if (!tracker->links)
    {
        MDV_LOGE("There is no memory for links");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, tracker->links);


    if (mdv_mutex_create(&tracker->links_mutex) != MDV_OK)
    {
        MDV_LOGE("links storage mutex not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &tracker->links_mutex);

    if (mdv_tracker_load(tracker) != MDV_OK)
    {
        MDV_LOGE("Tracker nodes loading failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    if (mdv_ebus_subscribe_all(tracker->ebus,
                               tracker,
                               mdv_tracker_handlers,
                               sizeof mdv_tracker_handlers / sizeof *mdv_tracker_handlers) != MDV_OK)
    {
        MDV_LOGE("Ebus subscription failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_free(rollbacker);

    return tracker;
}


static void mdv_tracker_free(mdv_tracker *tracker)
{
    mdv_ebus_unsubscribe_all(tracker->ebus,
                             tracker,
                             mdv_tracker_handlers,
                             sizeof mdv_tracker_handlers / sizeof *mdv_tracker_handlers);

    mdv_hashmap_release(tracker->links);
    mdv_mutex_free(&tracker->links_mutex);

    mdv_hashmap_release(tracker->ids);
    mdv_hashmap_release(tracker->nodes);
    mdv_mutex_free(&tracker->nodes_mutex);

    mdv_storage_release(tracker->storage);
    mdv_ebus_release(tracker->ebus);

    memset(tracker, 0, sizeof *tracker);

    mdv_free(tracker, "tracker");
}


mdv_tracker * mdv_tracker_retain(mdv_tracker *tracker)
{
    atomic_fetch_add_explicit(&tracker->rc, 1, memory_order_acquire);
    return tracker;
}


void mdv_tracker_release(mdv_tracker *tracker)
{
    if (tracker
        && atomic_fetch_sub_explicit(&tracker->rc, 1, memory_order_release) == 1)
    {
        mdv_tracker_free(tracker);
    }
}


mdv_uuid const * mdv_tracker_uuid(mdv_tracker *tracker)
{
    return &tracker->uuid;
}


static mdv_node * mdv_tracker_new_node(mdv_uuid const *uuid, uint32_t id, char const *addr)
{
    size_t const addr_len = strlen(addr);

    if (addr_len > MDV_ADDR_LEN_MAX)
    {
        MDV_LOGE("Invalid node address length");
        return 0;
    }

    static _Thread_local char buf[sizeof(mdv_node) + MDV_ADDR_LEN_MAX];

    mdv_node *new_node = (mdv_node *)buf;

    new_node->uuid = *uuid;
    new_node->id = id;
    memcpy(new_node->addr, addr, addr_len + 1);

    return new_node;
}


static mdv_errno mdv_tracker_register(mdv_tracker *tracker, mdv_uuid const *uuid, char const *addr)
{
    mdv_errno err = mdv_mutex_lock(&tracker->nodes_mutex);

    if (err == MDV_OK)
    {
        mdv_node *node = mdv_tracker_find(tracker, uuid);

        if (node)
        {
            if (strcmp(addr, node->addr) != 0)
            {
                mdv_node *new_node = mdv_tracker_new_node(uuid, node->id, addr);

                if (new_node)
                {
                    // Error is supressed because changed address is not very big problem
                    mdv_nodes_store(tracker->storage, new_node);

                    // Node address changed
                    err = mdv_tracker_insert(tracker, new_node)
                            ? MDV_OK
                            : MDV_FAILED;
                }
                else
                    err = MDV_FAILED;
            }
            else
                MDV_LOGD("Connection with '%s' is already exist", mdv_uuid_to_str(&node->uuid).ptr);
        }
        else
        {
            mdv_node *new_node = mdv_tracker_new_node(
                                        uuid,
                                        mdv_tracker_new_id(tracker),
                                        addr);

            if (new_node)
            {
                err = mdv_nodes_store(tracker->storage, new_node);

                if (err == MDV_OK)
                    err = mdv_tracker_insert(tracker, new_node)
                                ? MDV_OK
                                : MDV_FAILED;
            }
            else
                err = MDV_FAILED;
        }

        mdv_mutex_unlock(&tracker->nodes_mutex);
    }

    return err;
}


static mdv_errno mdv_tracker_linkstate2(mdv_tracker            *tracker,
                                        mdv_tracker_link const *link,
                                        bool                    connected)
{
    mdv_errno err = MDV_FAILED;

    if (mdv_mutex_lock(&tracker->links_mutex) == MDV_OK)
    {
        if (connected)
        {
            err = mdv_hashmap_insert(tracker->links, link, sizeof(*link))
                    ? MDV_OK
                    : MDV_NO_MEM;
        }
        else
        {
            mdv_hashmap_erase(tracker->links, link->id);
            err = MDV_OK;
        }

        mdv_mutex_unlock(&tracker->links_mutex);
    }

    return err;
}


// TODO: remove all links from isolated segment
static mdv_errno mdv_tracker_linkstate(mdv_tracker         *tracker,
                                       mdv_uuid const      *peer_1,
                                       mdv_uuid const      *peer_2,
                                       bool                 connected)
{
    mdv_errno err = MDV_FAILED;

    mdv_tracker_link link =
    {
        .id = { MDV_LOCAL_ID, MDV_LOCAL_ID },
        .weight = 1
    };

    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        do
        {
            if (mdv_uuid_cmp(peer_1, &tracker->uuid) != 0)
            {
                mdv_node *node = mdv_tracker_find(tracker, peer_1);

                if (!node)
                {
                    MDV_LOGE("Link registartion failed. Node '%s' not found.", mdv_uuid_to_str(peer_1).ptr);
                    break;
                }

                link.id[0] = node->id;
            }

            if (mdv_uuid_cmp(peer_2, &tracker->uuid) != 0)
            {
                mdv_node *node = mdv_tracker_find(tracker, peer_2);

                if (!node)
                {
                    MDV_LOGE("Link registartion failed. Node '%s' not found.", mdv_uuid_to_str(peer_2).ptr);
                    break;
                }

                link.id[1] = node->id;
            }

            err = MDV_OK;

        } while (0);

        mdv_mutex_unlock(&tracker->nodes_mutex);
    }

    if (err != MDV_OK)
        return err;

    return mdv_tracker_linkstate2(tracker, &link, connected);
}


size_t mdv_tracker_links_count(mdv_tracker *tracker)
{
    size_t links_count = 0;

    if (mdv_mutex_lock(&tracker->links_mutex) == MDV_OK)
    {
        links_count = mdv_hashmap_size(tracker->links);
        mdv_mutex_unlock(&tracker->links_mutex);
    }

    return links_count;
}


void mdv_tracker_links_foreach(mdv_tracker *tracker, void *arg, void (*fn)(mdv_tracker_link const *, void *))
{
    if (mdv_mutex_lock(&tracker->links_mutex) == MDV_OK)
    {
        mdv_hashmap_foreach(tracker->links, mdv_tracker_link, entry)
        {
            fn(entry, arg);
        }
        mdv_mutex_unlock(&tracker->links_mutex);
    }
}


mdv_node * mdv_tracker_node_by_id(mdv_tracker *tracker, uint32_t id)
{
    mdv_node *node = 0;

    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        mdv_node_id *node_id = mdv_hashmap_find(tracker->ids, &id);

        if (node_id)
        {
            static _Thread_local char buff[offsetof(mdv_node, addr) + MDV_ADDR_LEN_MAX + 1];

            size_t const node_size = mdv_node_size(node_id->node);

            if (node_size <= sizeof buff)
            {
                node = (mdv_node *)buff;
                memcpy(node, node_id->node, node_size);
            }
            else
                MDV_LOGE("Incorrect node size: %zd", node_size);
        }

        mdv_mutex_unlock(&tracker->nodes_mutex);
    }

    return node;
}


mdv_node * mdv_tracker_node_by_uuid(mdv_tracker *tracker, mdv_uuid const *uuid)
{
    mdv_node *ret = 0;

    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        mdv_node *node = mdv_hashmap_find(tracker->nodes, uuid);

        if (node)
        {
            static _Thread_local char buff[offsetof(mdv_node, addr) + MDV_ADDR_LEN_MAX + 1];

            size_t const node_size = mdv_node_size(node);

            if (node_size <= sizeof buff)
            {
                ret = (mdv_node *)buff;
                memcpy(ret, node, node_size);
            }
            else
                MDV_LOGE("Incorrect node size: %zd", node_size);
        }

        mdv_mutex_unlock(&tracker->nodes_mutex);
    }

    return ret;
}


mdv_vector * mdv_tracker_nodes(mdv_tracker *tracker)
{
    mdv_vector *ids = mdv_vector_create(atomic_load(&tracker->max_id), sizeof(uint32_t), &mdv_default_allocator);

    if (!ids)
    {
        MDV_LOGE("No memory for node identifiers");
        return 0;
    }

    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        mdv_hashmap_foreach(tracker->nodes, mdv_node, node)
        {
            if (!mdv_vector_push_back(ids, &node->id))
                MDV_LOGE("No memory for node identifier");
        }
        mdv_mutex_unlock(&tracker->nodes_mutex);
    }

    return ids;
}


mdv_vector * mdv_tracker_links(mdv_tracker *tracker)
{
    mdv_vector *links = &mdv_empty_vector;

    if (mdv_mutex_lock(&tracker->links_mutex) == MDV_OK)
    {
        size_t const links_count = mdv_hashmap_size(tracker->links);

        if (links_count)
        {
            links = mdv_vector_create(links_count,
                                      sizeof(mdv_tracker_link),
                                      &mdv_default_allocator);

            if (links)
            {
                mdv_hashmap_foreach(tracker->links, mdv_tracker_link, link)
                {
                    mdv_vector_push_back(links, link);
                }
            }
        }

        mdv_mutex_unlock(&tracker->links_mutex);
    }
    else
        return 0;

    if (!links)
        MDV_LOGE("No memory for links vector");

    return links;
}


typedef struct
{
    uint32_t    id;
    uint32_t    idx;
    mdv_uuid    uuid;
    char        addr[1];
} mdv_topology_node;


static mdv_topology_node const * mdv_topology_node_register(mdv_tracker *tracker,
                                                            mdv_hashmap *unique_nodes,
                                                            uint32_t     id)
{
    mdv_topology_node const *toponode = mdv_hashmap_find(unique_nodes, &id);

    if (!toponode)
    {
        mdv_node *node = mdv_tracker_node_by_id(tracker, id);

        if (!node)
        {
            MDV_LOGE("Node with %u identifier wasn't registered", id);
            return 0;
        }

        size_t const addr_size = strlen(node->addr) + 1;
        size_t const size = offsetof(mdv_topology_node, addr) + addr_size;

        char buf[size];

        mdv_topology_node *new_toponode = (void*)buf;

        new_toponode->id = id;
        new_toponode->uuid = node->uuid;

        memcpy(new_toponode->addr, node->addr, addr_size);

        toponode = mdv_hashmap_insert(unique_nodes, new_toponode, size);

        if (!toponode)
        {
            MDV_LOGE("No memory for node id");
            return 0;
        }
    }

    return toponode;
}


static size_t mdv_topology_extra_size(mdv_hashmap const *unique_nodes)
{
    size_t size = 0;

    mdv_hashmap_foreach(unique_nodes, mdv_topology_node, node)
    {
        size += strlen(node->addr) + 1;
    }

    return size;
}


static mdv_topology * mdv_tracker_topology(mdv_tracker *tracker)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(5);

    mdv_vector *links = mdv_tracker_links(tracker);

    if (!links)
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    if (mdv_vector_empty(links))
    {
        mdv_rollback(rollbacker);
        return &mdv_empty_topology;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, links);

    mdv_hashmap *unique_nodes = mdv_hashmap_create(mdv_topology_node,
                                                   id,
                                                   64,
                                                   mdv_u32_hash,
                                                   mdv_u32_keys_cmp);

    if (!unique_nodes)
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, unique_nodes);

    mdv_vector_foreach(links, mdv_tracker_link, link)
    {
        mdv_topology_node const *id0 = mdv_topology_node_register(tracker, unique_nodes, link->id[0]);
        mdv_topology_node const *id1 = mdv_topology_node_register(tracker, unique_nodes, link->id[1]);

        if (!id0 || !id1)
        {
            MDV_LOGE("Invalid network topology");
            mdv_rollback(rollbacker);
            return 0;
        }
    }

    mdv_vector *toponodes = mdv_vector_create(mdv_hashmap_size(unique_nodes),
                                              sizeof(mdv_toponode),
                                              &mdv_default_allocator);

    if (!toponodes)
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, toponodes);

    mdv_vector *topolinks = mdv_vector_create(mdv_vector_size(links),
                                              sizeof(mdv_topolink),
                                              &mdv_default_allocator);

    if (!topolinks)
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, topolinks);

    mdv_vector *extradata = mdv_vector_create(mdv_topology_extra_size(unique_nodes),
                                              sizeof(char),
                                              &mdv_default_allocator);

    if (!extradata)
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_vector_release, extradata);

    uint32_t n = 0;

    mdv_hashmap_foreach(unique_nodes, mdv_topology_node, node)
    {
        node->idx = n++;

        char const *addr = mdv_vector_append(extradata, node->addr, strlen(node->addr) + 1);

        mdv_toponode const toponode =
        {
            .uuid = node->uuid,
            .addr = addr
        };

        mdv_vector_push_back(toponodes, &toponode);
    }

    mdv_vector_foreach(links, mdv_tracker_link, link)
    {
        mdv_topology_node const *n0 = mdv_hashmap_find(unique_nodes, &link->id[0]);
        mdv_topology_node const *n1 = mdv_hashmap_find(unique_nodes, &link->id[1]);

        mdv_topolink const topolink =
        {
            .node = { n0->idx, n1->idx },
            .weight = link->weight
        };

        mdv_vector_push_back(topolinks, &topolink);
    }

    mdv_topology *topology = mdv_topology_create(toponodes, topolinks, extradata);

    if (!topology)
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_vector_release(toponodes);
    mdv_vector_release(topolinks);
    mdv_vector_release(extradata);

    mdv_hashmap_release(unique_nodes);
    mdv_vector_release(links);

    mdv_rollbacker_free(rollbacker);

    return topology;
}

