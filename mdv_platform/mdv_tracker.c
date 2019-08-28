#include "mdv_tracker.h"
#include "mdv_limits.h"
#include "mdv_log.h"
#include "mdv_rollbacker.h"
#include "mdv_algorithm.h"
#include <string.h>


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


static mdv_node * mdv_tracker_insert(mdv_tracker *tracker, mdv_node const *node);
static mdv_node * mdv_tracker_find(mdv_tracker *tracker, mdv_uuid const *uuid);
static void       mdv_tracker_erase(mdv_tracker *tracker, mdv_node const *node);
static mdv_errno  mdv_tracker_insert_peer(mdv_tracker *tracker, mdv_node *node);
static void       mdv_tracker_erase_peer(mdv_tracker *tracker, mdv_uuid const *uuid);


static size_t mdv_node_id_hash(uint32_t const *id)
{
    return *id;
}

static int mdv_node_id_cmp(uint32_t const *id1, uint32_t const *id2)
{
    return (int)*id1 - *id2;
}


static size_t mdv_link_hash(mdv_tracker_link const *link)
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


static int mdv_link_cmp(mdv_tracker_link const *link1, mdv_tracker_link const *link2)
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
    return ++tracker->max_id;
}


mdv_errno mdv_tracker_create(mdv_tracker *tracker, mdv_uuid const *uuid)
{
    mdv_rollbacker(8) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    tracker->uuid   = *uuid;
    tracker->max_id = 0;

    if (!mdv_hashmap_init(tracker->nodes,
                          mdv_node,
                          uuid,
                          256,
                          mdv_uuid_hash,
                          mdv_uuid_cmp))
    {
        MDV_LOGE("There is no memory for nodes");
        return MDV_NO_MEM;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &tracker->nodes);


    if (mdv_mutex_create(&tracker->nodes_mutex) != MDV_OK)
    {
        MDV_LOGE("Nodes storage mutex not created");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &tracker->nodes_mutex);


    if (!mdv_hashmap_init(tracker->ids,
                          mdv_node_id,
                          id,
                          256,
                          mdv_node_id_hash,
                          mdv_node_id_cmp))
    {
        MDV_LOGE("There is no memory for nodes");
        mdv_rollback(rollbacker);
        return MDV_NO_MEM;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &tracker->ids);


    if (mdv_mutex_create(&tracker->ids_mutex) != MDV_OK)
    {
        MDV_LOGE("Nodes storage mutex not created");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &tracker->ids_mutex);


    if (!mdv_hashmap_init(tracker->peers,
                          mdv_peer_id,
                          uuid,
                          256,
                          mdv_uuid_hash,
                          mdv_uuid_cmp))
    {
        MDV_LOGE("There is no memory for peers");
        mdv_rollback(rollbacker);
        return MDV_NO_MEM;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &tracker->peers);


    if (mdv_mutex_create(&tracker->peers_mutex) != MDV_OK)
    {
        MDV_LOGE("Peers storage mutex not created");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &tracker->peers_mutex);


    if (!mdv_hashmap_init(tracker->links,
                          mdv_tracker_link,
                          id,
                          256,
                          mdv_link_hash,
                          mdv_link_cmp))
    {
        MDV_LOGE("There is no memory for links");
        mdv_rollback(rollbacker);
        return MDV_NO_MEM;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &tracker->links);


    if (mdv_mutex_create(&tracker->links_mutex) != MDV_OK)
    {
        MDV_LOGE("links storage mutex not created");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &tracker->links_mutex);

    return MDV_OK;
}


void mdv_tracker_free(mdv_tracker *tracker)
{
    if (tracker)
    {
        mdv_hashmap_free(tracker->ids);
        mdv_mutex_free(&tracker->ids_mutex);

        mdv_hashmap_free(tracker->peers);
        mdv_mutex_free(&tracker->peers_mutex);

        mdv_hashmap_free(tracker->links);
        mdv_mutex_free(&tracker->links_mutex);

        mdv_hashmap_free(tracker->nodes);
        mdv_mutex_free(&tracker->nodes_mutex);

        memset(tracker, 0, sizeof *tracker);
    }
}


mdv_errno mdv_tracker_peer_connected(mdv_tracker *tracker, mdv_node *new_node)
{
    mdv_errno err = MDV_FAILED;

    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        if (mdv_mutex_lock(&tracker->ids_mutex) == MDV_OK)
        {
            if (mdv_mutex_lock(&tracker->peers_mutex) == MDV_OK)
            {
                mdv_node *node = mdv_tracker_find(tracker, &new_node->uuid);

                err = MDV_OK;

                if (node)
                {
                    new_node->id    = node->id;

                    if (!node->connected)
                    {
                        node->accepted  = new_node->accepted;
                        node->active    = 1;
                        node->userdata  = new_node->userdata;

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
                    new_node->active = 1;
                    err = mdv_tracker_insert(tracker, new_node) ? MDV_OK : MDV_FAILED;
                }

                mdv_mutex_unlock(&tracker->peers_mutex);
            }
            mdv_mutex_unlock(&tracker->ids_mutex);
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
            if (mdv_mutex_lock(&tracker->peers_mutex) == MDV_OK)
            {
                node->connected = 0;
                node->accepted  = 0;
                node->active    = 0;
                node->userdata  = 0;
                mdv_tracker_erase_peer(tracker, uuid);

                mdv_mutex_unlock(&tracker->peers_mutex);
            }
        }

        mdv_mutex_unlock(&tracker->nodes_mutex);
    }
}


void mdv_tracker_append(mdv_tracker *tracker, mdv_node const *node)
{
    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        if (!mdv_tracker_find(tracker, &node->uuid))
        {
            if (mdv_mutex_lock(&tracker->ids_mutex) == MDV_OK)
            {
                if (tracker->max_id < node->id)
                    tracker->max_id = node->id;

                mdv_tracker_insert(tracker, node);
                mdv_mutex_unlock(&tracker->ids_mutex);
            }
        }

        mdv_mutex_unlock(&tracker->nodes_mutex);
    }
}


void mdv_tracker_peers_foreach(mdv_tracker *tracker, void *arg, void (*fn)(mdv_node *, void *))
{
    if (mdv_mutex_lock(&tracker->peers_mutex) == MDV_OK)
    {
        mdv_hashmap_foreach(tracker->peers, mdv_peer_id, entry)
        {
            fn(entry->node, arg);
        }
        mdv_mutex_unlock(&tracker->peers_mutex);
    }
}


size_t mdv_tracker_peers_count(mdv_tracker *tracker)
{
    size_t peers_count = 0;

    if (mdv_mutex_lock(&tracker->peers_mutex) == MDV_OK)
    {
        peers_count = mdv_hashmap_size(tracker->peers);
        mdv_mutex_unlock(&tracker->peers_mutex);
    }

    return peers_count;
}


mdv_errno mdv_tracker_peers_call(mdv_tracker *tracker, uint32_t id, void *arg, mdv_errno (*fn)(mdv_node *, void *))
{
    mdv_errno err = MDV_FAILED;

    if (mdv_mutex_lock(&tracker->peers_mutex) == MDV_OK)
    {
        mdv_node_id *node_id = 0;

        if (mdv_mutex_lock(&tracker->ids_mutex) == MDV_OK)
        {
            node_id = mdv_hashmap_find(tracker->ids, id);
            mdv_mutex_unlock(&tracker->ids_mutex);
        }

        if (node_id)
        {
            mdv_node *node = node_id->node;

            if (node->connected)
                err = fn(node, arg);
        }

        mdv_mutex_unlock(&tracker->peers_mutex);
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


bool mdv_tracker_node_uuid(mdv_tracker *tracker, uint32_t id, mdv_uuid *uuid)
{
    if (id == MDV_LOCAL_ID)
    {
        *uuid = tracker->uuid;
        return true;
    }

    mdv_node_id *node_id = 0;

    if (mdv_mutex_lock(&tracker->ids_mutex) == MDV_OK)
    {
        node_id = mdv_hashmap_find(tracker->ids, id);
        mdv_mutex_unlock(&tracker->ids_mutex);
    }

    if (!node_id)
        return false;

    *uuid = node_id->node->uuid;

    return true;
}
