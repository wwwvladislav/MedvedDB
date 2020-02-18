#include "mdv_nodes.h"
#include "mdv_storages.h"
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <mdv_binn.h>
#include <mdv_limits.h>
#include <string.h>


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


static bool mdv_unbinn_node(binn const *obj, mdv_uuid const *uuid, mdv_node *node)
{
    char *addr = 0;
    uint32_t id;

    if (0
        || !binn_object_get_uint32((void*)obj, "I", &id)
        || !binn_object_get_str((void*)obj, "A", &addr))
    {
        MDV_LOGE("unbinn_node failed");
        return false;
    }

    size_t const addr_len = strlen(addr);

    if (addr_len >= MDV_ADDR_LEN_MAX)
    {
        MDV_LOGE("unbinn_node failed");
        return false;
    }

    memset(node, 0, sizeof *node);

    node->uuid      = *uuid;
    node->id        = id;

    memcpy(node->addr, addr, addr_len + 1);

    return true;
}


mdv_errno mdv_nodes_foreach(mdv_storage *storage,
                            void *arg,
                            void (*fn)(void *arg, mdv_node const *node))
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

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

    char buff[offsetof(mdv_node, addr) + MDV_ADDR_LEN_MAX + 1];

    mdv_node *node = (mdv_node *)buff;

    mdv_map_foreach(transaction, nodes_map, entry)
    {
        mdv_uuid const *uuid = (mdv_uuid const *)entry.key.ptr;

        binn obj;

        char uuid_str[MDV_UUID_STR_LEN];

        if(!binn_load(entry.value.ptr, &obj))
        {
            MDV_LOGW("Node '%s' discarded", mdv_uuid_to_str(uuid, uuid_str));
            continue;
        }

        if (mdv_unbinn_node(&obj, uuid, node))
        {
            MDV_LOGI("Node: [%u] %s, %s", node->id, mdv_uuid_to_str(&node->uuid, uuid_str), node->addr);
            fn(arg, node);
        }
        else
            MDV_LOGW("Node '%s' discarded", mdv_uuid_to_str(uuid, uuid_str));

        binn_free(&obj);
    }

    if (!mdv_transaction_commit(&transaction))
    {
        MDV_LOGE("Nodes storage transaction failed.");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_map_close(&nodes_map);

    mdv_rollbacker_free(rollbacker);

    return MDV_OK;
}


mdv_errno mdv_nodes_store(mdv_storage *storage, mdv_node const *node)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

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


    binn obj;

    if (!mdv_binn_node(node, &obj))
    {
        MDV_LOGE("Nodes storage transaction registration failed.");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_data const key =
    {
        .size = sizeof(node->uuid),
        .ptr = (void*)&node->uuid
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
        binn_free(&obj);
        return MDV_FAILED;
    }

    binn_free(&obj);

    if (!mdv_transaction_commit(&transaction))
    {
        MDV_LOGE("Nodes storage transaction failed.");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_map_close(&nodes_map);

    mdv_rollbacker_free(rollbacker);

    return MDV_OK;
}

