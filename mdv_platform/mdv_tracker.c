#include "mdv_tracker.h"
#include "mdv_limits.h"
#include "mdv_log.h"
#include "mdv_rollbacker.h"
#include <string.h>


/// Node identifier
typedef struct
{
    uint32_t   id;      ///< Unique identifier inside current server
    mdv_uuid   uuid;    ///< Global unique identifier
} mdv_node_id;


static mdv_errno  mdv_tracker_insert(mdv_tracker *tracker, mdv_node *node);
static mdv_node * mdv_tracker_find(mdv_tracker *tracker, mdv_uuid const *uuid);
static void       mdv_tracker_erase(mdv_tracker *tracker, mdv_node *node);


static size_t mdv_node_id_hash(int const *id)
{
    return *id;
}

static int mdv_node_id_cmp(int const *id1, int const *id2)
{
    return *id1 - *id2;
}


static mdv_errno mdv_tracker_insert(mdv_tracker *tracker, mdv_node *node)
{
    if (_mdv_hashmap_insert(&tracker->nodes, node, node->size))
    {
        mdv_node_id const nid =
        {
            .id = node->id,
            .uuid = node->uuid
        };

        if (mdv_hashmap_insert(tracker->ids, nid))
            return MDV_OK;

        mdv_hashmap_erase(tracker->nodes, node->uuid);
        MDV_LOGW("Node '%s' discarded", mdv_uuid_to_str(&node->uuid).ptr);
    }
    else
        MDV_LOGW("Node '%s' discarded", mdv_uuid_to_str(&node->uuid).ptr);

    return MDV_FAILED;
}


static void mdv_tracker_erase(mdv_tracker *tracker, mdv_node *node)
{
    mdv_hashmap_erase(tracker->nodes, node->uuid);
    mdv_hashmap_erase(tracker->ids, node->id);
}


static mdv_node * mdv_tracker_find(mdv_tracker *tracker, mdv_uuid const *uuid)
{
    return mdv_hashmap_find(tracker->nodes, *uuid);
}


static uint32_t mdv_tracker_new_id(mdv_tracker *tracker)
{
    return ++tracker->max_id;
}


mdv_errno mdv_tracker_create(mdv_tracker *tracker)
{
    mdv_rollbacker(6) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    tracker->max_id = 0;


    if (!mdv_hashmap_init(tracker->nodes,
                          mdv_node,
                          uuid,
                          256,
                          &mdv_uuid_hash,
                          &mdv_uuid_cmp))
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
                          &mdv_node_id_hash,
                          &mdv_node_id_cmp))
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

    return MDV_OK;
}


void mdv_tracker_free(mdv_tracker *tracker)
{
    if (tracker)
    {
        mdv_hashmap_free(tracker->nodes);
        mdv_mutex_free(&tracker->nodes_mutex);
        mdv_hashmap_free(tracker->ids);
        mdv_mutex_free(&tracker->ids_mutex);
        memset(tracker, 0, sizeof *tracker);
    }
}


mdv_errno mdv_tracker_reg(mdv_tracker *tracker, mdv_node *new_node)
{
    mdv_errno err = MDV_FAILED;

    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        if (mdv_mutex_lock(&tracker->ids_mutex) == MDV_OK)
        {
            mdv_node *node = mdv_tracker_find(tracker, &new_node->uuid);

            if (node)
            {
                new_node->id = node->id;
                node->active = 1;
                err = MDV_OK;
            }
            else
            {
                new_node->id = mdv_tracker_new_id(tracker);
                new_node->active = 1;
                err = mdv_tracker_insert(tracker, new_node);
            }

            mdv_mutex_unlock(&tracker->ids_mutex);
        }
        mdv_mutex_unlock(&tracker->nodes_mutex);
    }

    return err;
}


void mdv_tracker_append(mdv_tracker *tracker, mdv_node const *node)
{
    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        if (mdv_mutex_lock(&tracker->ids_mutex) == MDV_OK)
        {
            if (!mdv_tracker_find(tracker, &node->uuid))
            {
                if (tracker->max_id < node->id)
                    tracker->max_id = node->id;
            }

            mdv_mutex_unlock(&tracker->ids_mutex);
        }
        mdv_mutex_unlock(&tracker->nodes_mutex);
    }
}


void mdv_tracker_unreg(mdv_tracker *tracker, mdv_uuid const *uuid)
{
    if (mdv_mutex_lock(&tracker->nodes_mutex) == MDV_OK)
    {
        if (mdv_mutex_lock(&tracker->ids_mutex) == MDV_OK)
        {
            mdv_node *node = mdv_tracker_find(tracker, uuid);

            if (node)
                node->active = 0;

            mdv_mutex_unlock(&tracker->ids_mutex);
        }
        mdv_mutex_unlock(&tracker->nodes_mutex);
    }
}
