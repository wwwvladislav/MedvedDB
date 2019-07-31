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


static void mdv_gossip_peer_add(mdv_node *node, void *arg)
{
    mdv_gossip_peers_set *set = arg;

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


void mdv_gossip_peers_get(mdv_tracker *tracker, mdv_gossip_peers *peers)
{
    mdv_gossip_peers_set peers_set =
    {
        .capacity = peers->size,
        .data = peers
    };

    peers->size = 0;

    mdv_tracker_peers_foreach(tracker, &peers_set, &mdv_gossip_peer_add);

    qsort(peers->peers, peers->size, sizeof(*peers->peers), &mdv_gossip_peer_cmp);
}
