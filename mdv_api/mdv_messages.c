#include "mdv_messages.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <string.h>



char const * mdv_msg_name(uint32_t id)
{
    switch(id)
    {
        case mdv_msg_hello_id:  return "hello";
        case mdv_msg_status_id: return "status";
    }
    return "unknown";
}


void mdv_msg_free(void *msg)
{
    mdv_free(msg);
}


binn * mdv_binn_hello(mdv_msg_hello const *msg)
{
    binn *obj = binn_object();
    if (!obj)
    {
        MDV_LOGE("binn_hello failed");
        return obj;
    }

    if (!binn_object_set_uint32(obj, "V", msg->version)
        || !binn_object_set_blob(obj, "S", (void*)msg->signature, sizeof(msg->signature)))
    {
        binn_free(obj);
        MDV_LOGE("binn_hello failed");
        return 0;
    }

    return obj;
}


bool mdv_unbinn_hello(binn *obj, mdv_msg_hello *msg)
{
    uint8_t *signature;
    int      signature_size = 0;

    if (!binn_object_get_uint32(obj, "V", &msg->version)
        || !binn_object_get_blob(obj, "S", (void**)&signature, &signature_size))
    {
        MDV_LOGE("unbinn_hello failed");
        return false;
    }

    memcpy(msg->signature, signature, signature_size);

    return true;
}


binn * mdv_binn_status(mdv_msg_status const *msg)
{
    binn *obj = binn_object();
    if (!obj)
    {
        MDV_LOGE("binn_status failed");
        return obj;
    }

    if (!binn_object_set_int32(obj, "E", msg->err)
        || !binn_object_set_str(obj, "M", (void*)msg->message))
    {
        binn_free(obj);
        MDV_LOGE("binn_status failed");
        return 0;
    }

    return obj;
}


mdv_msg_status * mdv_unbinn_status(binn *obj)
{
    int   err;
    char *message;

    if (!binn_object_get_int32(obj, "E", &err)
        || !binn_object_get_str(obj, "M", &message))
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
