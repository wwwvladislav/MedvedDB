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
        case mdv_message_id(p2p_broadcast):     return "P2P BROADCAST";
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
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_p2p_linkstate failed");
        return false;
    }

    if (0
        || !binn_object_set_uint64(obj, "U1", msg->src.uuid.u64[0])
        || !binn_object_set_uint64(obj, "U2", msg->src.uuid.u64[1])
        || !binn_object_set_uint64(obj, "U3", msg->dst.uuid.u64[0])
        || !binn_object_set_uint64(obj, "U4", msg->dst.uuid.u64[1])
        || !binn_object_set_str(obj,    "A1", (char*)msg->src.addr)
        || !binn_object_set_str(obj,    "A2", (char*)msg->dst.addr)
        || !binn_object_set_bool(obj,   "F",  msg->connected))
    {
        binn_free(obj);
        MDV_LOGE("binn_p2p_linkstate failed");
        return false;
    }

    return true;
}


bool mdv_unbinn_p2p_linkstate(binn const *obj, mdv_msg_p2p_linkstate *msg)
{
    char *src_addr = 0,
         *dst_addr = 0;

    BOOL connected = 0;

    if (0
        || !binn_object_get_uint64((void*)obj, "U1", (uint64 *)&msg->src.uuid.u64[0])
        || !binn_object_get_uint64((void*)obj, "U2", (uint64 *)&msg->src.uuid.u64[1])
        || !binn_object_get_uint64((void*)obj, "U3", (uint64 *)&msg->dst.uuid.u64[0])
        || !binn_object_get_uint64((void*)obj, "U4", (uint64 *)&msg->dst.uuid.u64[1])
        || !binn_object_get_str((void*)obj,    "A1", &src_addr)
        || !binn_object_get_str((void*)obj,    "A2", &dst_addr)
        || !binn_object_get_bool((void*)obj,   "F2", &connected))
    {
        MDV_LOGE("unbinn_p2p_linkstate failed");
        return false;
    }

    msg->src.addr = src_addr;
    msg->dst.addr = dst_addr;
    msg->connected = !!connected;

    return true;
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
    mdv_topology_release(msg->topology);
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
    mdv_topology_release(msg->topology);
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

    mdv_list_foreach(msg->rows, mdv_cfslog_data, entry)
    {
        binn row;

        if (!binn_create_object(&row))
        {
            MDV_LOGE("binn_p2p_cfslog_data failed");
            binn_free(&rows);
            return false;
        }

        if (0
            || !binn_object_set_uint64(&row, "I", entry->row_id)
            || !binn_object_set_blob(&row, "O", entry->op.ptr, entry->op.size)
            || !binn_list_add_object(&rows, &row))
        {
            MDV_LOGE("binn_p2p_cfslog_data failed");
            binn_free(&row);
            binn_free(&rows);
            return false;
        }

        data_size += entry->op.size;

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


typedef mdv_list_entry(mdv_cfslog_data) mdv_cfslog_data_list_entry;


bool mdv_unbinn_p2p_cfslog_data_rows(binn const *obj, mdv_list *rows)
{
    binn *rows_list = 0;

    if (!binn_object_get_list((void*)obj, "R", (void**)&rows_list))
    {
        MDV_LOGE("unbinn_p2p_cfslog_data_rows failed");
        return false;
    }

    binn_iter iter = {};
    binn value = {};

    bool ret = true;

    binn_list_foreach(rows_list, value)
    {
        uint64_t row_id = 0;
        void *ptr = 0;
        int size = 0;

        if (!binn_object_get_uint64((void*)&value, "I", (uint64*)&row_id)
            || !binn_object_get_blob((void*)&value, "O", &ptr, &size))
        {
            ret = false;
            MDV_LOGE("unbinn_p2p_cfslog_data_count failed");
            break;
        }

        mdv_cfslog_data_list_entry *op = mdv_alloc(sizeof(mdv_cfslog_data_list_entry) + size, "cfstorage_op_list_entry");

        if (!op)
        {
            ret = false;
            MDV_LOGE("No memory for TR log entry");
            break;
        }

        op->data.row_id = row_id;
        op->data.op.size = size;
        op->data.op.ptr = op + 1;

        memcpy(op->data.op.ptr, ptr, size);

        mdv_list_emplace_back(rows, (mdv_list_entry_base *)op);
    }

    return ret;
}


bool mdv_binn_p2p_broadcast(mdv_msg_p2p_broadcast const *msg, binn *obj)
{
    binn notified;

    if (!binn_create_list(&notified))
    {
        MDV_LOGE("binn_p2p_broadcast failed");
        return false;
    }

    mdv_hashmap_foreach(msg->notified, mdv_uuid, entry)
    {
        binn uuid;
        uint8_t tmp[64];

        if (!binn_create(&uuid, BINN_OBJECT, sizeof tmp, tmp))
        {
            MDV_LOGE("binn_p2p_broadcast failed");
            binn_free(&notified);
            return false;
        }

        if (0
            || !binn_object_set_uint64(&uuid, "U1", entry->u64[0])
            || !binn_object_set_uint64(&uuid, "U2", entry->u64[1])
            || !binn_list_add_object(&notified, &uuid))
        {
            MDV_LOGE("binn_p2p_broadcast failed");
            binn_free(&uuid);
            binn_free(&notified);
            return false;
        }

        binn_free(&uuid);
    }

    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_p2p_broadcast failed");
        binn_free(&notified);
        return false;
    }

    if (0
        || !binn_object_set_uint16(obj, "M", msg->msg_id)
        || !binn_object_set_blob(obj,   "D", msg->data, (int)msg->size)
        || !binn_object_set_list(obj,   "N", &notified))
    {
        MDV_LOGE("binn_p2p_broadcast failed");
        binn_free(obj);
        binn_free(&notified);
        return false;
    }

    binn_free(&notified);

    return true;
}


bool mdv_unbinn_p2p_broadcast(binn const *obj, mdv_msg_p2p_broadcast *msg)
{
    int size = 0;

    binn *notified = 0;

    if (0
        || !binn_object_get_uint16((void*)obj, "M", &msg->msg_id)
        || !binn_object_get_blob((void*)obj,   "D", &msg->data, &size)
        || !binn_object_get_list((void*)obj,   "N", (void**)&notified))
    {
        MDV_LOGE("unbinn_p2p_broadcast failed");
        return false;
    }

    if (size < 0)
    {
        MDV_LOGE("unbinn_p2p_broadcast failed");
        return false;
    }

    msg->notified = _mdv_hashmap_create(
                                1,
                                0,
                                sizeof(mdv_uuid),
                                (mdv_hash_fn)mdv_uuid_hash,
                                (mdv_key_cmp_fn)mdv_uuid_cmp);

    if (!msg->notified)
    {
        MDV_LOGE("unbinn_p2p_broadcast failed. No memory.");
        return false;
    }

    msg->size = size;

    binn_iter iter = {};
    binn value = {};

    binn_list_foreach(notified, value)
    {
        mdv_uuid uuid;

        if (0
            || !binn_object_get_uint64((void*)&value, "U1", (uint64*)&uuid.u64[0])
            || !binn_object_get_uint64((void*)&value, "U2", (uint64*)&uuid.u64[1]))
        {
            MDV_LOGE("unbinn_p2p_broadcast failed");
            return false;
        }

        if (!mdv_hashmap_insert(msg->notified, &uuid, sizeof uuid))
        {
            MDV_LOGE("unbinn_p2p_broadcast failed");
            return false;
        }
    }

    return true;
}


