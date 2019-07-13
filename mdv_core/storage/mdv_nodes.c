#include "mdv_nodes.h"
#include "mdv_storages.h"
#include "../mdv_config.h"
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <mdv_binn.h>
#include <mdv_limits.h>
#include <stdbool.h>
#include <string.h>


/// Node identifier
typedef struct
{
    uint32_t   id;      ///< Unique identifier inside current server
    mdv_uuid   uuid;    ///< Global unique identifier
} mdv_node_id;


static bool mdv_nodes_insert(mdv_nodes *nodes, mdv_node *node);
static bool mdv_nodes_find(mdv_nodes *nodes, mdv_uuid const *uuid, uint32_t *id);
static void mdv_nodes_erase(mdv_nodes *nodes, mdv_node *node);


static size_t mdv_node_id_hash(int const *id)
{
    return *id;
}

static int mdv_node_id_cmp(int const *id1, int const *id2)
{
    return *id1 - *id2;
}


static bool mdv_binn_node(mdv_node const *node, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_node failed");
        return false;
    }

    if (0
        || !binn_object_set_uint32(obj, "I", node->id)
        || !binn_object_set_str(obj, "A", (char*)node->addr))
    {
        binn_free(obj);
        MDV_LOGE("binn_node failed");
        return false;
    }

    return true;
}


static mdv_node * mdv_unbinn_node(binn const *obj, mdv_uuid const *uuid)
{
    char *addr = 0;
    uint32_t id;

    if (0
        || !binn_object_get_uint32((void*)obj, "I", &id)
        || !binn_object_get_str((void*)obj, "A", &addr))
    {
        MDV_LOGE("unbinn_node failed");
        return 0;
    }

    size_t const addr_len = strlen(addr);

    if (addr_len >= MDV_ADDR_LEN_MAX)
    {
        MDV_LOGE("unbinn_node failed");
        return 0;
    }

    static _Thread_local char buff[offsetof(mdv_node, addr) + MDV_ADDR_LEN_MAX];

    mdv_node *node = (mdv_node *)buff;

    node->size = offsetof(mdv_node, addr) + addr_len + 1;
    node->uuid = *uuid;
    node->id = id;
    memcpy(node->addr, addr, addr_len + 1);

    return node;
}


static uint32_t mdv_nodes_new_id(mdv_nodes *nodes)
{
    return ++nodes->max_id;
}


mdv_errno mdv_nodes_load(mdv_nodes *nodes, mdv_storage *storage)
{
    mdv_rollbacker(6) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    nodes->max_id = 0;


    if (!mdv_hashmap_init(nodes->nodes,
                          mdv_node,
                          uuid,
                          256,
                          &mdv_uuid_hash,
                          &mdv_uuid_cmp))
    {
        MDV_LOGE("There is no memory for nodes");
        return MDV_NO_MEM;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &nodes->nodes);


    nodes->nodes_mutex = mdv_mutex_create();

    if (!nodes->nodes_mutex)
    {
        MDV_LOGE("Nodes storage mutex not created");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, nodes->nodes_mutex);


    if (!mdv_hashmap_init(nodes->ids,
                          mdv_node_id,
                          id,
                          256,
                          &mdv_node_id_hash,
                          &mdv_node_id_cmp))
    {
        MDV_LOGE("There is no memory for nodes");
        mdv_rollback(rollbacker);
        return MDV_NO_MEM;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &nodes->ids);


    nodes->ids_mutex = mdv_mutex_create();

    if (!nodes->ids_mutex)
    {
        MDV_LOGE("Nodes storage mutex not created");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, nodes->ids_mutex);


    // Start transaction
    mdv_transaction transaction = mdv_transaction_start(storage);
    if (!mdv_transaction_ok(transaction))
    {
        MDV_LOGE("Nodes storage transaction not started");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

    // Open nodes map
    mdv_map nodes_map = mdv_map_open(&transaction, MDV_MAP_NODES, MDV_MAP_CREATE);

    if (!mdv_map_ok(nodes_map))
    {
        MDV_LOGE("Table '%s' not opened", MDV_MAP_NODES);
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_map_close, &nodes_map);

    mdv_map_foreach(transaction, nodes_map, entry)
    {
        mdv_uuid const *uuid = (mdv_uuid const *)entry.key.ptr;

        binn obj;

        if(!binn_load(entry.value.ptr, &obj))
        {
            MDV_LOGW("Node '%s' discarded", mdv_uuid_to_str(uuid).ptr);
            continue;
        }

        mdv_node *node = mdv_unbinn_node(&obj, uuid);

        if (node)
        {
            if (node->id > nodes->max_id)
                nodes->max_id = node->id;

            MDV_LOGI("Node: [%u] %s, %s", node->id, mdv_uuid_to_str(&node->uuid).ptr, node->addr);

            mdv_nodes_insert(nodes, node);
        }
        else
            MDV_LOGW("Node '%s' discarded", mdv_uuid_to_str(uuid).ptr);

        binn_free(&obj);
    }

    if (!mdv_transaction_commit(&transaction))
    {
        MDV_LOGE("Nodes storage transaction failed.");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_map_close(&nodes_map);

    nodes->storage = mdv_storage_retain(storage);

    return MDV_OK;
}


void mdv_nodes_free(mdv_nodes *nodes)
{
    if (nodes)
    {
        mdv_hashmap_free(nodes->nodes);
        mdv_mutex_free(nodes->nodes_mutex);
        mdv_hashmap_free(nodes->ids);
        mdv_mutex_free(nodes->ids_mutex);
        mdv_storage_release(nodes->storage);
    }
}


static bool mdv_nodes_insert(mdv_nodes *nodes, mdv_node *node)
{
    if (_mdv_hashmap_insert(&nodes->nodes, node, node->size))
    {
        mdv_node_id const nid =
        {
            .id = node->id,
            .uuid = node->uuid
        };

        if (mdv_hashmap_insert(nodes->ids, nid))
            return true;

        mdv_hashmap_erase(nodes->nodes, node->uuid);
        MDV_LOGW("Node '%s' discarded", mdv_uuid_to_str(&node->uuid).ptr);
    }
    else
        MDV_LOGW("Node '%s' discarded", mdv_uuid_to_str(&node->uuid).ptr);

    return false;
}


static void mdv_nodes_erase(mdv_nodes *nodes, mdv_node *node)
{
    mdv_hashmap_erase(nodes->nodes, node->uuid);
    mdv_hashmap_erase(nodes->ids, node->id);
}


static bool mdv_nodes_find(mdv_nodes *nodes, mdv_uuid const *uuid, uint32_t *id)
{
    mdv_node *node = mdv_hashmap_find(nodes->nodes, *uuid);
    if (node)
    {
        *id = node->id;
        return true;
    }
    return false;
}


mdv_errno mdv_nodes_reg(mdv_nodes *nodes, mdv_node *node)
{
    mdv_errno err = MDV_FAILED;

    if (mdv_mutex_lock(nodes->nodes_mutex) == MDV_OK)
    {
        if (mdv_mutex_lock(nodes->ids_mutex) == MDV_OK)
        {
            if (!mdv_nodes_find(nodes, &node->uuid, &node->id))
                node->id = mdv_nodes_new_id(nodes);

            do
            {
                if (mdv_nodes_insert(nodes, node))
                {
                    // Insert into DB
                    mdv_rollbacker(2) rollbacker;
                    mdv_rollbacker_clear(rollbacker);

                    // Start transaction
                    mdv_transaction transaction = mdv_transaction_start(nodes->storage);
                    if (!mdv_transaction_ok(transaction))
                    {
                        MDV_LOGE("Nodes storage transaction not started");
                        mdv_rollback(rollbacker);
                        mdv_nodes_erase(nodes, node);
                        break;
                    }

                    mdv_rollbacker_push(rollbacker, mdv_transaction_abort, &transaction);

                    // Open nodes map
                    mdv_map nodes_map = mdv_map_open(&transaction, MDV_MAP_NODES, MDV_MAP_CREATE);

                    if (!mdv_map_ok(nodes_map))
                    {
                        MDV_LOGE("Table '%s' not opened", MDV_MAP_NODES);
                        mdv_rollback(rollbacker);
                        mdv_nodes_erase(nodes, node);
                        break;
                    }

                    mdv_rollbacker_push(rollbacker, mdv_map_close, &nodes_map);

                    binn obj;

                    if (!mdv_binn_node(node, &obj))
                    {
                        MDV_LOGE("Nodes storage transaction registration failed.");
                        mdv_rollback(rollbacker);
                        mdv_nodes_erase(nodes, node);
                        break;
                    }

                    mdv_data const key =
                    {
                        .size = sizeof(node->uuid),
                        .ptr = &node->uuid
                    };

                    mdv_data const value =
                    {
                        .size = binn_size(&obj),
                        .ptr = binn_ptr(&obj)
                    };

                    if (!mdv_map_put(&nodes_map, &transaction, &key, &value))
                    {
                        MDV_LOGE("Nodes storage transaction registration failed.");
                        mdv_rollback(rollbacker);
                        mdv_nodes_erase(nodes, node);
                        binn_free(&obj);
                        break;
                    }

                    binn_free(&obj);

                    if (mdv_transaction_commit(&transaction))
                        err = MDV_OK;
                    else
                    {
                        MDV_LOGE("Nodes storage transaction failed.");
                        mdv_rollback(rollbacker);
                        mdv_nodes_erase(nodes, node);
                    }

                    mdv_map_close(&nodes_map);
                }
            } while(0);

            mdv_mutex_unlock(nodes->ids_mutex);
        }
        mdv_mutex_unlock(nodes->nodes_mutex);
    }

    return err;
}

