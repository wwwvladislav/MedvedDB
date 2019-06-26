#include "mdv_nodes.h"
#include "../mdv_config.h"


static uint32_t mdv_nodes_new_id(mdv_nodes *nodes)
{
    return atomic_fetch_add_explicit(&nodes->max_id, 1, memory_order_relaxed) + 1;
}


bool mdv_nodes_load(mdv_nodes *nodes, mdv_storage *storage)
{
    nodes->nodes = mdv_deque_create(sizeof(mdv_node));
    nodes->storage = 0;
    atomic_init(&nodes->max_id, 0);

    if (!nodes->nodes)
        return false;

    nodes->storage = mdv_storage_retain(storage);

    // TODO: Load nodes from DB

    return true;
}


void mdv_nodes_free(mdv_nodes *nodes)
{
    if (nodes)
    {
        mdv_deque_free(nodes->nodes);
        mdv_storage_release(nodes->storage);
    }
}


bool mdv_nodes_add(mdv_nodes *nodes, size_t size, mdv_node const *new_nodes)
{
    return true;
}

