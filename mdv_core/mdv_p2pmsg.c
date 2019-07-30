#include "mdv_p2pmsg.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <string.h>


char const * mdv_p2p_msg_name(uint32_t id)
{
    switch(id)
    {
        case mdv_message_id(p2p_hello):     return "P2P HELLO";
        case mdv_message_id(p2p_linkstate): return "P2P LINK STATE";
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

    mdv_msg_p2p_hello *msg = (mdv_msg_p2p_hello *)mdv_alloc(sizeof(mdv_msg_p2p_hello) + listen_len + 1, "msg_p2p_hello");

    if (!msg)
    {
        MDV_LOGE("unbinn_p2p_hello failed");
        return 0;
    }

    msg->version = version;
    msg->uuid = uuid;
    msg->listen = (char*)(msg + 1);

    memcpy(msg->listen, listen, listen_len + 1);

    return msg;
}


bool mdv_binn_p2p_linkstate(mdv_msg_p2p_linkstate const *msg, binn *obj)
{
    binn peers;

    if (!binn_create_list(&peers))
    {
        MDV_LOGE("binn_p2p_linkstate failed");
        return false;
    }

    for(uint32_t i = 0; i < msg->peers_count; ++i)
    {
        if (!binn_list_add_uint32(&peers, msg->peers[i]))
        {
            binn_free(&peers);
            MDV_LOGE("binn_p2p_linkstate failed");
            return false;
        }
    }

    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_p2p_linkstate failed");
        binn_free(&peers);
        return false;
    }

    uint8_t const flags = msg->connected;

    if (0
        || !binn_object_set_uint64(obj, "U1", msg->peer_1.u64[0])
        || !binn_object_set_uint64(obj, "U2", msg->peer_1.u64[1])
        || !binn_object_set_uint64(obj, "U3", msg->peer_2.u64[0])
        || !binn_object_set_uint64(obj, "U4", msg->peer_2.u64[1])
        || !binn_object_set_uint8(obj, "F", flags)
        || !binn_object_set_uint32(obj, "S", msg->peers_count)
        || !binn_object_set_list(obj, "P", &peers))
    {
        binn_free(obj);
        binn_free(&peers);
        MDV_LOGE("binn_p2p_linkstate failed");
        return false;
    }

    binn_free(&peers);

    return true;
}


mdv_msg_p2p_linkstate * mdv_unbinn_p2p_linkstate(binn const *obj)
{
    uint32_t peers_count = 0;

    if (!binn_object_get_uint32((void*)obj, "S", &peers_count))
    {
        MDV_LOGE("unbinn_p2p_linkstate failed");
        return 0;
    }

    mdv_msg_p2p_linkstate *msg = mdv_alloc(sizeof(mdv_msg_p2p_linkstate) + peers_count * sizeof(uint32_t), "msg_p2p_linkstate");

    if (!msg)
    {
        MDV_LOGE("unbinn_p2p_linkstate failed. No memory.");
        return 0;
    }

    uint8_t flags = 0;

    if (0
        || !binn_object_get_uint64((void*)obj, "U1", (uint64 *)&msg->peer_1.u64[0])
        || !binn_object_get_uint64((void*)obj, "U2", (uint64 *)&msg->peer_1.u64[1])
        || !binn_object_get_uint64((void*)obj, "U3", (uint64 *)&msg->peer_2.u64[0])
        || !binn_object_get_uint64((void*)obj, "U4", (uint64 *)&msg->peer_2.u64[1])
        || !binn_object_get_uint8((void*)obj, "F", (unsigned char*)&flags))
    {
        MDV_LOGE("unbinn_p2p_linkstate failed");
        mdv_free(msg, "msg_p2p_linkstate");
        return 0;
    }

    msg->connected = flags & 1;
    msg->peers_count = peers_count;
    msg->peers = (uint32_t *)(msg + 1);

    binn *peers = 0;

    if (!binn_object_get_list((void*)obj, "P", (void**)&peers))
    {
        MDV_LOGE("unbinn_p2p_linkstate failed");
        mdv_free(msg, "msg_p2p_linkstate");
        return 0;
    }

    binn_iter iter;
    binn value;
    size_t i = 0;

    binn_list_foreach(peers, value)
    {
        if (i > peers_count)
        {
            MDV_LOGE("unbinn_p2p_linkstate failed");
            mdv_free(msg, "msg_p2p_linkstate");
            return 0;
        }

        int64 v = 0;

        if (!binn_get_int64(&value, &v))
        {
            MDV_LOGE("unbinn_p2p_linkstate failed");
            mdv_free(msg, "msg_p2p_linkstate");
            return 0;
        }

        msg->peers[i] = (uint32_t)v;

        ++i;
    }

    return msg;
}
