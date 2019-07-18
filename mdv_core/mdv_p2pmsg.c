#include "mdv_p2pmsg.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <string.h>


char const * mdv_p2p_msg_name(uint32_t id)
{
    switch(id)
    {
        case mdv_message_id(p2p_hello):     return "P2P HELLO";
    }

    return "P2P UNKOWN";
}


bool mdv_binn_p2p_hello(mdv_msg_p2p_hello const *msg, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_p2p_hello failed");
        return false;
    }

    if (0
        || !binn_object_set_uint32(obj, "V", msg->version)
        || !binn_object_set_uint64(obj, "U0", msg->uuid.u64[0])
        || !binn_object_set_uint64(obj, "U1", msg->uuid.u64[1])
        || !binn_object_set_str(obj, "L", (char*)msg->listen))
    {
        binn_free(obj);
        MDV_LOGE("binn_p2p_hello failed");
        return false;
    }

    return true;

}


mdv_msg_p2p_hello * mdv_unbinn_p2p_hello(binn const *obj)
{
    uint32_t    version;
    mdv_uuid    uuid;
    char       *listen;

    if (0
        || !binn_object_get_uint32((void*)obj, "V", &version)
        || !binn_object_get_uint64((void*)obj, "U0", (uint64 *)&uuid.u64[0])
        || !binn_object_get_uint64((void*)obj, "U1", (uint64 *)&uuid.u64[1])
        || !binn_object_get_str((void*)obj, "L", &listen))
    {
        MDV_LOGE("unbinn_p2p_hello failed");
        return 0;
    }

    if (!listen)
    {
        MDV_LOGE("unbinn_p2p_hello failed");
        return 0;
    }

    size_t const listen_len = strlen(listen);

    mdv_msg_p2p_hello *msg = (mdv_msg_p2p_hello *)mdv_alloc(offsetof(mdv_msg_p2p_hello, dataspace) + listen_len + 1, "msg_p2p_hello");

    if (!msg)
    {
        MDV_LOGE("unbinn_p2p_hello failed");
        return 0;
    }

    msg->version = version;
    msg->uuid = uuid;
    msg->listen = msg->dataspace;

    memcpy(msg->listen, listen, listen_len + 1);

    return msg;
}
