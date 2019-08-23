#include "mdv_messages.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <mdv_serialization.h>
#include <mdv_rollbacker.h>
#include <string.h>


char const * mdv_msg_name(uint32_t id)
{
    switch(id)
    {
        case mdv_message_id(hello):         return "HELLO";
        case mdv_message_id(status):        return "STATUS";
        case mdv_message_id(create_table):  return "CREATE TABLE";
        case mdv_message_id(table_info):    return "TABLE INFO";
        case mdv_message_id(get_topology):  return "GET TOPOLOGY";
        case mdv_message_id(topology):      return "TOPOLOGY";
    }
    return "UNKOWN";
}


bool mdv_binn_hello(mdv_msg_hello const *msg, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_hello failed");
        return false;
    }

    if (0
        || !binn_object_set_uint32(obj, "V", msg->version)
        || !binn_object_set_uint64(obj, "U0", msg->uuid.u64[0])
        || !binn_object_set_uint64(obj, "U1", msg->uuid.u64[1]))
    {
        binn_free(obj);
        MDV_LOGE("binn_hello failed");
        return false;
    }

    return true;
}


bool mdv_unbinn_hello(binn const *obj, mdv_msg_hello *msg)
{
    if (0
        || !binn_object_get_uint32((void*)obj, "V", &msg->version)
        || !binn_object_get_uint64((void*)obj, "U0", (uint64 *)(msg->uuid.u64 + 0))
        || !binn_object_get_uint64((void*)obj, "U1", (uint64 *)(msg->uuid.u64 + 1)))
    {
        MDV_LOGE("unbinn_hello failed");
        return false;
    }

    return true;
}


bool mdv_binn_status(mdv_msg_status const *msg, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_status failed");
        return false;
    }

    if (!binn_object_set_int32(obj, "E", msg->err)
        || !binn_object_set_str(obj, "M", (void*)msg->message))
    {
        binn_free(obj);
        MDV_LOGE("binn_status failed");
        return false;
    }

    return true;
}


bool mdv_unbinn_status(binn const *obj, mdv_msg_status *msg)
{
    char *message = 0;

    if (!binn_object_get_int32((void*)obj, "E", &msg->err)
        || !binn_object_get_str((void*)obj, "M", &message))
    {
        MDV_LOGE("unbinn_status failed");
        return false;
    }

    msg->message = message;

    return true;
}


bool mdv_binn_create_table(mdv_msg_create_table_base const *msg, binn *obj)
{
    return mdv_binn_table((mdv_table_base const *)&msg->table, obj);
}


mdv_msg_create_table_base * mdv_unbinn_create_table (binn const *obj)
{
    return (mdv_msg_create_table_base *)mdv_unbinn_table(obj);
}


bool mdv_binn_table_info(mdv_msg_table_info const *msg, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_table_info failed");
        return false;
    }

    if (0
        || !binn_object_set_uint64(obj, "0", msg->uuid.u64[0])
        || !binn_object_set_uint64(obj, "1", msg->uuid.u64[1]))
    {
        binn_free(obj);
        MDV_LOGE("binn_table_info failed");
        return false;
    }

    return true;
}


bool mdv_unbinn_table_info(binn const *obj, mdv_msg_table_info *msg)
{
    if (0
        || !binn_object_get_uint64((void*)obj, "0", (uint64 *)(msg->uuid.u64 + 0))
        || !binn_object_get_uint64((void*)obj, "1", (uint64 *)(msg->uuid.u64 + 1)))
    {
        MDV_LOGE("unbinn_table_info failed");
        return false;
    }

    return true;
}


bool mdv_binn_get_topology(mdv_msg_get_topology const *msg, binn *obj)
{
    (void)msg;

    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_get_topology failed");
        return false;
    }

    return true;
}


bool mdv_unbinn_get_topology(binn const *obj, mdv_msg_get_topology *msg)
{
    (void)msg;
    return true;
}


bool mdv_binn_topology(mdv_msg_topology const *msg, binn *obj)
{
    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    binn nodes;
    binn links;

    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_topology failed");
        return false;
    }

    mdv_rollbacker_push(rollbacker, binn_free, obj);

    if (!binn_create_list(&nodes))
    {
        MDV_LOGE("binn_topology failed");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &nodes);

    if (!binn_create_list(&links))
    {
        MDV_LOGE("binn_topology failed");
        mdv_rollback(rollbacker);
        return false;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &links);

    for(uint32_t i = 0; i < msg->nodes_count; ++i)
    {
        if (0
            ||!binn_list_add_uint64(&nodes, msg->nodes[i].uuid.u64[0])
            ||!binn_list_add_uint64(&nodes, msg->nodes[i].uuid.u64[1]))
        {
            MDV_LOGE("binn_topology failed");
            mdv_rollback(rollbacker);
            return false;
        }
    }

    for(uint32_t i = 0; i < msg->links_count; ++i)
    {
        uint64_t const n0 = (uint64_t)(msg->links[i].node[0] - msg->nodes);
        uint64_t const n1 = (uint64_t)(msg->links[i].node[1] - msg->nodes);

        if (0
            ||!binn_list_add_uint64(&links, n0)
            ||!binn_list_add_uint64(&links, n1))
        {
            MDV_LOGE("binn_topology failed");
            mdv_rollback(rollbacker);
            return false;
        }
    }

    if (0
        || !binn_object_set_uint64(obj, "NC", msg->nodes_count)
        || !binn_object_set_uint64(obj, "LC", msg->links_count)
        || !binn_object_set_list(obj, "N", &nodes)
        || !binn_object_set_list(obj, "L", &links))
    {
        MDV_LOGE("binn_topology failed");
        mdv_rollback(rollbacker);
        return false;
    }

    binn_free(&nodes);
    binn_free(&links);

    return true;
}


uint64_t * mdv_unbinn_topology_nodes_count(binn const *obj)
{
    static _Thread_local uint64_t size;

    if (!binn_object_get_uint64((void*)obj, "NC", (uint64 *)&size))
    {
        MDV_LOGE("unbinn_topology_nodes_count failed");
        return 0;
    }

    return &size;
}


uint64_t * mdv_unbinn_topology_links_count(binn const *obj)
{
    static _Thread_local uint64_t size;

    if (!binn_object_get_uint64((void*)obj, "LC", (uint64 *)&size))
    {
        MDV_LOGE("unbinn_topology_links_count failed");
        return 0;
    }

    return &size;
}


bool mdv_unbinn_topology(binn const *obj, mdv_msg_topology *msg)
{
    uint64_t nodes_count = 0;
    uint64_t links_count = 0;
    binn    *nodes = 0;
    binn    *links = 0;

    if (0
        || !binn_object_get_uint64((void*)obj, "NC", (uint64*)&nodes_count)
        || !binn_object_get_uint64((void*)obj, "LC", (uint64*)&links_count)
        || !binn_object_get_list((void*)obj, "N", (void**)&nodes)
        || !binn_object_get_list((void*)obj, "L", (void**)&links)
        || msg->nodes_count != nodes_count
        || msg->links_count != links_count)
    {
        MDV_LOGE("unbinn_topology failed");
        return false;
    }

    binn_iter iter;
    binn value;
    size_t i;

    // load nodes
    i = 0;
    binn_list_foreach(nodes, value)
    {
        size_t const n = i / 2;
        size_t const p = i - n * 2;

        if (n > nodes_count)
        {
            MDV_LOGE("unbinn_topology failed");
            return false;
        }

        mdv_toponode *node = msg->nodes + n;

        if (!binn_get_int64(&value, (int64*)&node->uuid.u64[p]))
        {
            MDV_LOGE("unbinn_topology_links failed");
            return false;
        }

        ++i;
    }

    // load links
    i = 0;
    binn_list_foreach(links, value)
    {
        size_t const n = i / 2;
        size_t const p = i - n * 2;

        if (n > links_count)
        {
            MDV_LOGE("unbinn_topology failed");
            return false;
        }

        int64 id = 0;

        if (!binn_get_int64(&value, &id)
            ||id >= nodes_count)
        {
            MDV_LOGE("unbinn_topology failed");
            return false;
        }

        msg->links[n].node[p] = msg->nodes + id;

        ++i;
    }

    return true;
}
