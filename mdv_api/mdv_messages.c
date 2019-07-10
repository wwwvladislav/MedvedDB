#include "mdv_messages.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <mdv_serialization.h>
#include <string.h>


char const * mdv_msg_name(uint32_t id)
{
    switch(id)
    {
        case mdv_msg_hello_id:          return "HELLO";
        case mdv_msg_status_id:         return "STATUS";
        case mdv_msg_create_table_id:   return "CREATE TABLE";
    }
    return "unknown";
}


void mdv_msg_free(void *msg)
{
    mdv_free(msg);
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


mdv_msg_status * mdv_unbinn_status(binn const *obj)
{
    int   err;
    char *message;

    if (!binn_object_get_int32((void*)obj, "E", &err)
        || !binn_object_get_str((void*)obj, "M", &message))
    {
        MDV_LOGE("unbinn_status failed");
        return 0;
    }

    int const message_len = strlen(message);

    mdv_msg_status *msg = (mdv_msg_status *)mdv_alloc(offsetof(mdv_msg_status, message) + message_len + 1);
    if (!msg)
    {
        MDV_LOGE("unbinn_status failed");
        return 0;
    }

    msg->err = err;
    memcpy(msg->message, message, message_len);
    msg->message[message_len] = 0;

    return msg;
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

