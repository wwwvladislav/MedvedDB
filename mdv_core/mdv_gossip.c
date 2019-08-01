#include "mdv_gossip.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <mdv_algorithm.h>
#include <mdv_hash.h>
#include <stdlib.h>


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


void mdv_gossip_peers_free(mdv_gossip_peers *peers)
{
    if (peers)
        mdv_free(peers, "gossip_peers");
}


static void mdv_gossip_peer_add(mdv_node *node, void *arg)
{
    mdv_gossip_peers_set *set = arg;

    if (set->data->size >= set->capacity)
    {
        // Most likely that reallocation isn't needed.
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


mdv_gossip_peers * mdv_gossip_peers_get(mdv_tracker *tracker)
{
    size_t const peers_count = mdv_tracker_peers_count(tracker) + 16;

    mdv_gossip_peers_set peers_set =
    {
        .capacity = peers_count,
        .data = mdv_gossip_peers_alloc(peers_count)
    };

    if (peers_set.data)
    {
        peers_set.data->size = 0;
        mdv_tracker_peers_foreach(tracker, &peers_set, &mdv_gossip_peer_add);
        qsort(peers_set.data->peers, peers_set.data->size, sizeof(*peers_set.data->peers), &mdv_gossip_peer_cmp);
    }

    return peers_set.data;
}

