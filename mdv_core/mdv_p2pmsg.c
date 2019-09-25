#include "mdv_p2pmsg.h"
#include <mdv_log.h>
#include <mdv_serialization.h>


char const * mdv_p2p_msg_name(uint32_t id)
{
    switch(id)
    {
        case mdv_message_id(p2p_hello):         return "P2P HELLO";
        case mdv_message_id(p2p_status):        return "P2P STATUS";
        case mdv_message_id(p2p_linkstate):     return "P2P LINK STATE";
        case mdv_message_id(p2p_toposync):      return "P2P TOPOLOGY SYNC";
        case mdv_message_id(p2p_topodiff):      return "P2P TOPOLOGY DIFF";
        case mdv_message_id(p2p_cfslog_sync):   return "P2P CFSLOG SYNC";
        case mdv_message_id(p2p_cfslog_state):  return "P2P CFSLOG STATE";
        case mdv_message_id(p2p_cfslog_data):   return "P2P CFSLOG DATA";
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


bool mdv_binn_p2p_status(mdv_msg_p2p_status const *msg, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_p2p_status failed");
        return false;
    }

    if (!binn_object_set_int32(obj, "E", msg->err)
        || !binn_object_set_str(obj, "M", (void*)msg->message))
    {
        binn_free(obj);
        MDV_LOGE("binn_p2p_status failed");
        return false;
    }

    return true;
}


bool mdv_unbinn_p2p_status(binn const *obj, mdv_msg_p2p_status *msg)
{
    char *message = 0;

    if (!binn_object_get_int32((void*)obj, "E", &msg->err)
        || !binn_object_get_str((void*)obj, "M", &message))
    {
        MDV_LOGE("unbinn_p2p_status failed");
        return false;
    }

    msg->message = message;

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
        || !binn_object_set_uint64(obj, "U1", msg->src.uuid.u64[0])
        || !binn_object_set_uint64(obj, "U2", msg->src.uuid.u64[1])
        || !binn_object_set_uint64(obj, "U3", msg->dst.uuid.u64[0])
        || !binn_object_set_uint64(obj, "U4", msg->dst.uuid.u64[1])
        || !binn_object_set_str(obj,    "A1", (char*)msg->src.addr)
        || !binn_object_set_str(obj,    "A2", (char*)msg->dst.addr)
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


mdv_toponode * mdv_unbinn_p2p_linkstate_src(binn const *obj)
{
    static _Thread_local mdv_toponode toponode;

    char *addr = 0;

    if (0
        || !binn_object_get_uint64((void*)obj, "U1", (uint64 *)&toponode.uuid.u64[0])
        || !binn_object_get_uint64((void*)obj, "U2", (uint64 *)&toponode.uuid.u64[1])
        || !binn_object_get_str((void*)obj, "A1", &addr))
    {
        MDV_LOGE("p2p_linkstate_src_peer failed");
        return 0;
    }

    toponode.addr = addr;

    return &toponode;
}


mdv_toponode * mdv_unbinn_p2p_linkstate_dst(binn const *obj)
{
    static _Thread_local mdv_toponode toponode;

    char *addr = 0;

    if (0
        || !binn_object_get_uint64((void*)obj, "U3", (uint64 *)&toponode.uuid.u64[0])
        || !binn_object_get_uint64((void*)obj, "U4", (uint64 *)&toponode.uuid.u64[1])
        || !binn_object_get_str((void*)obj, "A2", &addr))
    {
        MDV_LOGE("p2p_linkstate_dst_peer failed");
        return 0;
    }

    toponode.addr = addr;

    return &toponode;
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

    binn_iter iter = {};
    binn value = {};
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


bool mdv_binn_p2p_topodiff(mdv_msg_p2p_topodiff const *msg, binn *obj)
{
    return mdv_topology_serialize(msg->topology, obj);
}


bool mdv_unbinn_p2p_topodiff(binn const *obj, mdv_msg_p2p_topodiff *msg)
{
    msg->topology = mdv_topology_deserialize(obj);
    return msg->topology != 0;
}


void mdv_p2p_topodiff_free(mdv_msg_p2p_topodiff *msg)
{
    mdv_topology_free(msg->topology);
    msg->topology = 0;
}


bool mdv_binn_p2p_cfslog_sync(mdv_msg_p2p_cfslog_sync const *msg, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_p2p_cfslog_sync failed");
        return false;
    }

    if (0
        || !binn_object_set_uint64(obj, "U0", msg->uuid.u64[0])
        || !binn_object_set_uint64(obj, "U1", msg->uuid.u64[1])
        || !binn_object_set_uint64(obj, "P0", msg->peer.u64[0])
        || !binn_object_set_uint64(obj, "P1", msg->peer.u64[1]))
    {
        binn_free(obj);
        MDV_LOGE("binn_p2p_cfslog_sync failed");
        return false;
    }

    return true;
}


bool mdv_unbinn_p2p_cfslog_sync(binn const *obj, mdv_msg_p2p_cfslog_sync *msg)
{
    if (0
        || !binn_object_get_uint64((void*)obj, "U0", (uint64 *)&msg->uuid.u64[0])
        || !binn_object_get_uint64((void*)obj, "U1", (uint64 *)&msg->uuid.u64[1])
        || !binn_object_get_uint64((void*)obj, "P0", (uint64 *)&msg->peer.u64[0])
        || !binn_object_get_uint64((void*)obj, "P1", (uint64 *)&msg->peer.u64[1]))
    {
        MDV_LOGE("unbinn_p2p_cfslog_sync failed");
        return false;
    }

    return true;
}


bool mdv_binn_p2p_cfslog_state(mdv_msg_p2p_cfslog_state const *msg, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_p2p_cfslog_state failed");
        return false;
    }

    if (0
        || !binn_object_set_uint64(obj, "U0", msg->uuid.u64[0])
        || !binn_object_set_uint64(obj, "U1", msg->uuid.u64[1])
        || !binn_object_set_uint64(obj, "P0", msg->peer.u64[0])
        || !binn_object_set_uint64(obj, "P1", msg->peer.u64[1])
        || !binn_object_set_uint64(obj, "T", msg->trlog_top))
    {
        binn_free(obj);
        MDV_LOGE("binn_p2p_cfslog_state failed");
        return false;
    }

    return true;

}


bool mdv_unbinn_p2p_cfslog_state(binn const *obj, mdv_msg_p2p_cfslog_state *msg)
{
    if (0
        || !binn_object_get_uint64((void*)obj, "U0", (uint64 *)&msg->uuid.u64[0])
        || !binn_object_get_uint64((void*)obj, "U1", (uint64 *)&msg->uuid.u64[1])
        || !binn_object_get_uint64((void*)obj, "P0", (uint64 *)&msg->peer.u64[0])
        || !binn_object_get_uint64((void*)obj, "P1", (uint64 *)&msg->peer.u64[1])
        || !binn_object_get_uint64((void*)obj, "T", (uint64 *)&msg->trlog_top))
    {
        MDV_LOGE("unbinn_p2p_cfslog_state failed");
        return false;
    }

    return true;
}


bool mdv_binn_p2p_cfslog_data(mdv_msg_p2p_cfslog_data const *msg, binn *obj)
{
    binn rows;

    if (!binn_create_list(&rows))
    {
        MDV_LOGE("binn_p2p_cfslog_data failed");
        return false;
    }

    uint64_t data_size = 0;

    for(uint32_t i = 0; i < msg->count; ++i)
    {
        binn row;

        if (!binn_create_object(&row))
        {
            MDV_LOGE("binn_p2p_cfslog_data failed");
            binn_free(&rows);
            return false;
        }

        if (0
            || !binn_object_set_uint64(&row, "I", msg->rows[i].row_id)
            || !binn_object_set_blob(&row, "O", msg->rows[i].op.ptr, msg->rows[i].op.size)
            || !binn_list_add_object(&rows, &row))
        {
            MDV_LOGE("binn_p2p_cfslog_data failed");
            binn_free(&row);
            binn_free(&rows);
            return false;
        }

        data_size += msg->rows[i].op.size;

        binn_free(&row);
    }

    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_p2p_cfslog_data failed");
        binn_free(&rows);
        return false;
    }

    if (0
        || !binn_object_set_uint64(obj, "U0", msg->uuid.u64[0])
        || !binn_object_set_uint64(obj, "U1", msg->uuid.u64[1])
        || !binn_object_set_uint64(obj, "P0", msg->peer.u64[0])
        || !binn_object_set_uint64(obj, "P1", msg->peer.u64[1])
        || !binn_object_set_uint32(obj, "C", msg->count)
        || !binn_object_set_uint64(obj, "S", data_size)
        || !binn_object_set_list(obj, "R", &rows))
    {
        binn_free(&rows);
        binn_free(obj);
        MDV_LOGE("binn_p2p_cfslog_data failed");
        return false;
    }

    binn_free(&rows);

    return true;
}


mdv_uuid * mdv_unbinn_p2p_cfslog_data_uuid(binn const *obj)
{
    static _Thread_local mdv_uuid uuid;

    if (!binn_object_get_uint64((void*)obj, "U0", (uint64*)&uuid.u64[0])
        || !binn_object_get_uint64((void*)obj, "U1", (uint64*)&uuid.u64[1]))
    {
        MDV_LOGE("unbinn_p2p_cfslog_data_uuid failed");
        return 0;
    }

    return &uuid;
}


mdv_uuid * mdv_unbinn_p2p_cfslog_data_peer(binn const *obj)
{
    static _Thread_local mdv_uuid uuid;

    if (!binn_object_get_uint64((void*)obj, "P0", (uint64*)&uuid.u64[0])
        || !binn_object_get_uint64((void*)obj, "P1", (uint64*)&uuid.u64[1]))
    {
        MDV_LOGE("unbinn_p2p_cfslog_data_uuid failed");
        return 0;
    }

    return &uuid;
}


uint32_t * mdv_unbinn_p2p_cfslog_data_count(binn const *obj)
{
    static _Thread_local uint32_t rows_count;

    if (!binn_object_get_uint32((void*)obj, "C", &rows_count))
    {
        MDV_LOGE("unbinn_p2p_cfslog_data_count failed");
        return 0;
    }

    return &rows_count;
}


uint64_t * mdv_unbinn_p2p_cfslog_data_size(binn const *obj)
{
    static _Thread_local uint64_t data_size;

    if (!binn_object_get_uint64((void*)obj, "S", (uint64*)&data_size))
    {
        MDV_LOGE("unbinn_p2p_cfslog_data_size failed");
        return 0;
    }

    return &data_size;
}


bool mdv_unbinn_p2p_cfslog_data_rows(binn const *obj,
                                     mdv_cfslog_data *rows, uint32_t rows_count,
                                     uint8_t *dataspace, size_t dataspace_size)
{
    binn *rows_list = 0;

    if (!binn_object_get_list((void*)obj, "R", (void**)&rows_list))
    {
        MDV_LOGE("unbinn_p2p_cfslog_data_rows failed");
        return false;
    }

    binn_iter iter = {};
    binn value = {};
    size_t i = 0;

    binn_list_foreach(rows_list, value)
    {
        if (i > rows_count)
        {
            MDV_LOGE("unbinn_p2p_cfslog_data_rows failed");
            return false;
        }

        void *ptr = 0;
        int size = 0;

        if (!binn_object_get_uint64((void*)&value, "I", (uint64*)&rows[i].row_id)
            || !binn_object_get_blob((void*)&value, "O", &ptr, &size))
        {
            MDV_LOGE("unbinn_p2p_cfslog_data_count failed");
            return 0;
        }

        if (dataspace_size < size)
        {
            MDV_LOGE("unbinn_p2p_cfslog_data_count failed");
            return 0;
        }

        rows[i].op.size = (uint32_t)size;
        rows[i].op.ptr = dataspace;

        memcpy(dataspace, ptr, size);

        dataspace += size;
        dataspace_size -= size;

        ++i;
    }

    return i == rows_count;
}
