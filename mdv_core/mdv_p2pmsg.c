#include "mdv_p2pmsg.h"
#include <mdv_log.h>
#include <mdv_serialization.h>


char const * mdv_p2p_msg_name(uint32_t id)
{
    switch(id)
    {
        case mdv_message_id(p2p_hello):     return "P2P HELLO";
        case mdv_message_id(p2p_linkstate): return "P2P LINK STATE";
        case mdv_message_id(p2p_toposync):  return "P2P TOPOLOGY SYNC";
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


bool mdv_unbinn_p2p_hello(binn const *obj, mdv_msg_p2p_hello *msg)
{
    if (0
        || !binn_object_get_uint32((void*)obj, "V", &msg->version)
        || !binn_object_get_uint64((void*)obj, "U0", (uint64 *)&msg->uuid.u64[0])
        || !binn_object_get_uint64((void*)obj, "U1", (uint64 *)&msg->uuid.u64[1])
        || !binn_object_get_str((void*)obj, "L", &msg->listen))
    {
        MDV_LOGE("unbinn_p2p_hello failed");
        return false;
    }

    return true;
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
        || !binn_object_set_uint64(obj, "U1", msg->src_peer.u64[0])
        || !binn_object_set_uint64(obj, "U2", msg->src_peer.u64[1])
        || !binn_object_set_uint64(obj, "U3", msg->dst_peer.u64[0])
        || !binn_object_set_uint64(obj, "U4", msg->dst_peer.u64[1])
        || !binn_object_set_str(obj,    "L", (char*)msg->src_listen)
        || !binn_object_set_uint8(obj,  "F", flags)
        || !binn_object_set_uint32(obj, "S", msg->peers_count)
        || !binn_object_set_list(obj,   "P", &peers))
    {
        binn_free(obj);
        binn_free(&peers);
        MDV_LOGE("binn_p2p_linkstate failed");
        return false;
    }

    binn_free(&peers);

    return true;
}


mdv_uuid * mdv_unbinn_p2p_linkstate_src_peer(binn const *obj)
{
    static _Thread_local mdv_uuid uuid;

    if (0
        || !binn_object_get_uint64((void*)obj, "U1", (uint64 *)&uuid.u64[0])
        || !binn_object_get_uint64((void*)obj, "U2", (uint64 *)&uuid.u64[1]))
    {
        MDV_LOGE("p2p_linkstate_src_peer failed");
        return 0;
    }

    return &uuid;
}


mdv_uuid * mdv_unbinn_p2p_linkstate_dst_peer(binn const *obj)
{
    static _Thread_local mdv_uuid uuid;

    if (0
        || !binn_object_get_uint64((void*)obj, "U3", (uint64 *)&uuid.u64[0])
        || !binn_object_get_uint64((void*)obj, "U4", (uint64 *)&uuid.u64[1]))
    {
        MDV_LOGE("p2p_linkstate_dst_peer failed");
        return 0;
    }

    return &uuid;
}


char const * mdv_unbinn_p2p_linkstate_src_listen(binn const *obj)
{
    char *listen = 0;

    if (!binn_object_get_str((void*)obj, "L", &listen))
    {
        MDV_LOGE("unbinn_p2p_linkstate_src_listen failed");
        return 0;
    }

    return listen;
}


bool * mdv_unbinn_p2p_linkstate_connected(binn const *obj)
{
    uint8_t flags = 0;

    if (!binn_object_get_uint8((void*)obj, "F", (unsigned char*)&flags))
    {
        MDV_LOGE("p2p_linkstate_connected failed");
        return 0;
    }

    static _Thread_local bool connected;

    connected = flags & 1;

    return &connected;
}


uint32_t * mdv_unbinn_p2p_linkstate_peers_count(binn const *obj)
{
    static _Thread_local uint32_t peers_count;

    if (!binn_object_get_uint32((void*)obj, "S", &peers_count))
    {
        MDV_LOGE("p2p_linkstate_peers_count failed");
        return 0;
    }

    return &peers_count;
}


bool mdv_unbinn_p2p_linkstate_peers(binn const *obj, uint32_t *peers, uint32_t peers_count)
{
    binn *binn_peers = 0;

    if (!binn_object_get_list((void*)obj, "P", (void**)&binn_peers))
    {
        MDV_LOGE("unbinn_p2p_linkstate failed");
        return false;
    }

    binn_iter iter;
    binn value;
    size_t i = 0;

    binn_list_foreach(binn_peers, value)
    {
        if (i > peers_count)
        {
            MDV_LOGE("unbinn_p2p_linkstate failed");
            return false;
        }

        int64 v = 0;

        if (!binn_get_int64(&value, &v))
        {
            MDV_LOGE("unbinn_p2p_linkstate failed");
            return false;
        }

        peers[i] = (uint32_t)v;

        ++i;
    }

    return i == peers_count;
}


bool mdv_binn_p2p_toposync(mdv_msg_p2p_toposync const *msg, binn *obj)
{
    return mdv_topology_serialize(msg->topology, obj);
}


bool mdv_unbinn_p2p_toposync(binn const *obj, mdv_msg_p2p_toposync *msg)
{
    msg->topology = mdv_topology_deserialize(obj);
    return msg->topology != 0;
}

void mdv_p2p_toposync_free(mdv_msg_p2p_toposync *msg)
{
    mdv_topology_free(msg->topology);
    msg->topology = 0;
}
