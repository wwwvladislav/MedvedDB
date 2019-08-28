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
    return mdv_topology_serialize(msg->topology, obj);
}


mdv_topology * mdv_unbinn_topology(binn const *obj)
{
    return mdv_topology_deserialize(obj);
}
