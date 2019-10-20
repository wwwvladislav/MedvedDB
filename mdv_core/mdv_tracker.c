#include "mdv_tracker.h"
#include "mdv_limits.h"
#include "mdv_log.h"
#include "mdv_rollbacker.h"
#include "mdv_algorithm.h"
#include <string.h>
#include <stdatomic.h>
#include <stdlib.h>


/// Node identifier
typedef struct
{
    uint32_t   id;      ///< Unique identifier inside current server
    mdv_node  *node;    ///< Cluster node information
} mdv_node_id;


/// Peer identifier
typedef struct
{
    mdv_uuid   uuid;    ///< Global unique identifier
    mdv_node  *node;    ///< Cluster node information
} mdv_peer_id;



/// Nodes and network topology tracker
struct mdv_tracker
{
    atomic_uint_fast32_t    rc;             ///< References counter
    mdv_uuid                uuid;           ///< Global unique identifier for current node (i.e. self UUID)
    atomic_uint_fast32_t    max_id;         ///< Maximum node identifier

    mdv_mutex               nodes_mutex;    ///< nodes guard mutex
    mdv_hashmap             nodes;          ///< Nodes map (UUID -> mdv_node)
    mdv_hashmap             ids;            ///< Node identifiers (id -> mdv_node *)
    mdv_hashmap             peers;          ///< Peers (i.e. connected neighbours. UUID -> mdv_node *)

    mdv_hashmap             links;          ///< Links (id -> id's vector)
    mdv_mutex               links_mutex;    ///< Links guard mutex
};


static mdv_node * mdv_tracker_insert(mdv_tracker *tracker, mdv_node const *node);
static mdv_node * mdv_tracker_find(mdv_tracker *tracker, mdv_uuid const *uuid);
static void       mdv_tracker_erase(mdv_tracker *tracker, mdv_node const *node);
static mdv_errno  mdv_tracker_insert_peer(mdv_tracker *tracker, mdv_node *node);
static void       mdv_tracker_erase_peer(mdv_tracker *tracker, mdv_uuid const *uuid);


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
    mdv_list_entry(mdv_node) *entry = (void*)_mdv_hashmap_insert(&tracker->nodes, node, node->size);

    if (entry)
    {
        mdv_node_id const nid =
        {
            .id = node->id,
            .node = &entry->data
        };

        if (mdv_hashmap_insert(tracker->ids, nid))
        {
            if (mdv_tracker_insert_peer(tracker, &entry->data) == MDV_OK)
                return &entry->data;

            mdv_hashmap_erase(tracker->ids, node->id);
        }

        mdv_hashmap_erase(tracker->nodes, node->uuid);
        MDV_LOGW("Node '%s' discarded", mdv_uuid_to_str(&node->uuid).ptr);
    }
    else
        MDV_LOGW("Node '%s' discarded", mdv_uuid_to_str(&node->uuid).ptr);

    return 0;
}


static mdv_errno mdv_tracker_insert_peer(mdv_tracker *tracker, mdv_node *node)
{
    if (node->connected)
    {
        mdv_peer_id const peer_id =
        {
            .uuid = node->uuid,
            .node = node
        };

        if (mdv_hashmap_insert(tracker->peers, peer_id))
            return MDV_OK;

        return MDV_FAILED;
    }

    return MDV_OK;
}


static void mdv_tracker_erase(mdv_tracker *tracker, mdv_node const *node)
{
    mdv_hashmap_erase(tracker->peers, node->uuid);
    mdv_hashmap_erase(tracker->ids, node->id);
    mdv_hashmap_erase(tracker->nodes, node->uuid);
}


static void mdv_tracker_erase_peer(mdv_tracker *tracker, mdv_uuid const *uuid)
{
    mdv_hashmap_erase(tracker->peers, uuid);
}


static mdv_node * mdv_tracker_find(mdv_tracker *tracker, mdv_uuid const *uuid)
{
    return mdv_hashmap_find(tracker->nodes, *uuid);
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


mdv_tracker * mdv_tracker_create(mdv_uuid const *uuid)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(7);

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

    if (!mdv_hashmap_init(tracker->nodes,
                          mdv_node,
                          uuid,
                          256,
                          mdv_uuid_hash,
                          mdv_uuid_cmp))
    {
        MDV_LOGE("There is no memory for nodes");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &tracker->nodes);


    if (mdv_mutex_create(&tracker->nodes_mutex) != MDV_OK)
    {
        MDV_LOGE("Nodes storage mutex not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &tracker->nodes_mutex);


    if (!mdv_hashmap_init(tracker->ids,
                          mdv_node_id,
                          id,
                          256,
                          mdv_u32_hash,
                          mdv_u32_keys_cmp))
    {
        MDV_LOGE("There is no memory for nodes");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &tracker->ids);


    if (!mdv_hashmap_init(tracker->peers,
                          mdv_peer_id,
                          uuid,
                          256,
                          mdv_uuid_hash,
                          mdv_uuid_cmp))
    {
        MDV_LOGE("There is no memory for peers");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &tracker->peers);


    if (!mdv_hashmap_init(tracker->links,
                          mdv_tracker_link,
                          id,
                          256,
                          mdv_tracker_link_hash,
                          mdv_tracker_link_cmp))
    {
        MDV_LOGE("There is no memory for links");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &tracker->links);


    if (mdv_mutex_create(&tracker->links_mutex) != MDV_OK)
    {
        MDV_LOGE("links storage mutex not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &tracker->links_mutex);

    mdv_rollbacker_free(rollbacker);

    return tracker;
}


static void mdv_tracker_free(mdv_tracker *tracker)
{
    mdv_hashmap_free(tracker->links);
    mdv_mutex_free(&tracker->links_mutex);

    mdv_hashmap_free(tracker->ids);
    mdv_hashmap_free(tracker->peers);
    mdv_hashmap_free(tracker->nodes);
    mdv_mutex_free(&tracker->nodes_mutex);

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


mdv_errno mdv_tracker_peer_connected(mdv_tracker *tracker, mdv_node *new_node)
{
    mdv_errno err = MDV_FAILED;

    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        mdv_node *node = mdv_tracker_find(tracker, &new_node->uuid);

        err = MDV_OK;

        if (node)
        {
            new_node->id    = node->id;

            if (!node->connected)
            {
                if (strcmp(new_node->addr, node->addr) != 0)
                {
                    // Node address changed
                    err = mdv_tracker_insert(tracker, new_node) ? MDV_OK : MDV_FAILED;
                }
                else if (new_node->connected)
                {
                    // Connection status changed
                    node->connected = 1;
                    err = mdv_tracker_insert_peer(tracker, node);
                }
            }
            else
            {
                err = MDV_EEXIST;
                MDV_LOGD("Connection with '%s' is already exist", mdv_uuid_to_str(&node->uuid).ptr);
            }
        }
        else
        {
            new_node->id = mdv_tracker_new_id(tracker);
            err = mdv_tracker_insert(tracker, new_node) ? MDV_OK : MDV_FAILED;
        }

        mdv_mutex_unlock(&tracker->nodes_mutex);
    }

    return err;
}


void mdv_tracker_peer_disconnected(mdv_tracker *tracker, mdv_uuid const *uuid)
{
    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        mdv_node *node = mdv_tracker_find(tracker, uuid);

        if (node)
        {
            node->connected = 0;
            mdv_tracker_erase_peer(tracker, uuid);
        }

        mdv_mutex_unlock(&tracker->nodes_mutex);
    }
}


bool mdv_tracker_append(mdv_tracker *tracker, mdv_node *new_node, bool is_new)
{
    bool is_added = false;

    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        mdv_node *node = mdv_tracker_find(tracker, &new_node->uuid);

        if (!node)
        {
            if (is_new)
                new_node->id = mdv_tracker_new_id(tracker);
            else
                mdv_tracker_id_maximize(tracker, new_node->id);

            mdv_tracker_insert(tracker, new_node);

            is_added = true;
        }

        mdv_mutex_unlock(&tracker->nodes_mutex);
    }

    return is_added;
}


void mdv_tracker_peers_foreach(mdv_tracker *tracker, void *arg, void (*fn)(mdv_node *, void *))
{
    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        mdv_hashmap_foreach(tracker->peers, mdv_peer_id, entry)
        {
            fn(entry->node, arg);
        }
        mdv_mutex_unlock(&tracker->nodes_mutex);
    }
}


size_t mdv_tracker_peers_count(mdv_tracker *tracker)
{
    size_t peers_count = 0;

    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        peers_count = mdv_hashmap_size(tracker->peers);
        mdv_mutex_unlock(&tracker->nodes_mutex);
    }

    return peers_count;
}


mdv_errno mdv_tracker_peers_call(mdv_tracker *tracker, uint32_t id, void *arg, mdv_errno (*fn)(mdv_node *, void *))
{
    mdv_errno err = MDV_FAILED;

    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        mdv_node_id *node_id = mdv_hashmap_find(tracker->ids, id);

        if (node_id)
        {
            mdv_node *node = node_id->node;

            if (node->connected)
                err = fn(node, arg);
        }

        mdv_mutex_unlock(&tracker->nodes_mutex);
    }

    return err;
}


mdv_errno mdv_tracker_linkstate(mdv_tracker         *tracker,
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


mdv_errno mdv_tracker_linkstate2(mdv_tracker            *tracker,
                                 mdv_tracker_link const *link,
                                 bool                    connected)
{
    mdv_errno err = MDV_FAILED;

    if (mdv_mutex_lock(&tracker->links_mutex) == MDV_OK)
    {
        if (connected)
        {
            err = mdv_hashmap_insert(tracker->links, *link)
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
        mdv_node_id *node_id = mdv_hashmap_find(tracker->ids, id);

        if (node_id)
        {
            static _Thread_local char buff[offsetof(mdv_node, addr) + MDV_ADDR_LEN_MAX + 1];

            if (node_id->node->size <= sizeof buff)
            {
                node = (mdv_node *)buff;
                memcpy(node, node_id->node, node_id->node->size);
            }
            else
                MDV_LOGE("Incorrect node size: %zd", node_id->node->size);
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
        mdv_node *node = mdv_hashmap_find(tracker->nodes, *uuid);

        if (node)
        {
            static _Thread_local char buff[offsetof(mdv_node, addr) + MDV_ADDR_LEN_MAX + 1];

            if (node->size <= sizeof buff)
            {
                ret = (mdv_node *)buff;
                memcpy(ret, node, node->size);
            }
            else
                MDV_LOGE("Incorrect node size: %zd", node->size);
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
    mdv_topology_node const *toponode = mdv_hashmap_find(*unique_nodes, id);

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

        mdv_list_entry_base *entry = _mdv_hashmap_insert(unique_nodes, new_toponode, size);

        if (!entry)
        {
            MDV_LOGE("No memory for node id");
            return 0;
        }

        toponode = (void*)entry->data;
    }

    return toponode;
}


static size_t mdv_topology_extra_size(mdv_hashmap const *unique_nodes)
{
    size_t size = 0;

    mdv_hashmap_foreach(*unique_nodes, mdv_topology_node, node)
    {
        size += strlen(node->addr) + 1;
    }

    return size;
}


mdv_topology * mdv_tracker_topology(mdv_tracker *tracker)
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

    mdv_hashmap unique_nodes;

    if (!mdv_hashmap_init(unique_nodes,
                          mdv_topology_node,
                          id,
                          64,
                          mdv_u32_hash,
                          mdv_u32_keys_cmp))
    {
        MDV_LOGE("No memory for network topology");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &unique_nodes);

    mdv_vector_foreach(links, mdv_tracker_link, link)
    {
        mdv_topology_node const *id0 = mdv_topology_node_register(tracker, &unique_nodes, link->id[0]);
        mdv_topology_node const *id1 = mdv_topology_node_register(tracker, &unique_nodes, link->id[1]);

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

    mdv_vector *extradata = mdv_vector_create(mdv_topology_extra_size(&unique_nodes),
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
        mdv_topology_node const *n0 = mdv_hashmap_find(unique_nodes, link->id[0]);
        mdv_topology_node const *n1 = mdv_hashmap_find(unique_nodes, link->id[1]);

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

    mdv_hashmap_free(unique_nodes);
    mdv_vector_release(links);

    mdv_rollbacker_free(rollbacker);

    return topology;
}
