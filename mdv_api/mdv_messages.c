#include "mdv_messages.h"
#include <mdv_log.h>
#include <mdv_serialization.h>


char const * mdv_msg_name(uint32_t id)
{
    switch(id)
    {
        case mdv_message_id(hello):         return "HELLO";
        case mdv_message_id(status):        return "STATUS";
        case mdv_message_id(create_table):  return "CREATE TABLE";
        case mdv_message_id(get_table):     return "GET TABLE";
        case mdv_message_id(table_info):    return "TABLE INFO";
        case mdv_message_id(table_desc):    return "TABLE DESC";
        case mdv_message_id(get_topology):  return "GET TOPOLOGY";
        case mdv_message_id(topology):      return "TOPOLOGY";
        case mdv_message_id(insert_into):   return "INSERT INTO";
        case mdv_message_id(select):        return "SELECT";
        case mdv_message_id(view):          return "VIEW";
        case mdv_message_id(fetch):         return "FETCH";
        case mdv_message_id(rowset):        return "ROWSET";
    }
    return "UNKOWN";
}


bool mdv_msg_hello_binn(mdv_msg_hello const *msg, binn *obj)
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


bool mdv_msg_hello_unbinn(binn const *obj, mdv_msg_hello *msg)
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


bool mdv_msg_status_binn(mdv_msg_status const *msg, binn *obj)
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


bool mdv_msg_status_unbinn(binn const *obj, mdv_msg_status *msg)
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


bool mdv_msg_create_table_binn(mdv_msg_create_table const *msg, binn *obj)
{
    return mdv_binn_table_desc(msg->desc, obj);
}


bool mdv_msg_create_table_unbinn(binn const *obj, mdv_msg_create_table *msg)
{
    msg->desc = mdv_unbinn_table_desc(obj);
    return msg != 0;
}


void mdv_msg_create_table_free(mdv_msg_create_table *msg)
{
    if (msg->desc)
    {
        mdv_free(msg->desc, "table_desc");
        msg->desc = 0;
    }
}


bool mdv_msg_get_table_binn(mdv_msg_get_table const *msg, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_get_table failed");
        return false;
    }

    if (0
        || !binn_object_set_uint64(obj, "0", msg->id.u64[0])
        || !binn_object_set_uint64(obj, "1", msg->id.u64[1]))
    {
        binn_free(obj);
        MDV_LOGE("binn_get_table failed");
        return false;
    }

    return true;
}


bool mdv_msg_get_table_unbinn(binn const *obj, mdv_msg_get_table *msg)
{
    if (0
        || !binn_object_get_uint64((void*)obj, "0", (uint64 *)(msg->id.u64 + 0))
        || !binn_object_get_uint64((void*)obj, "1", (uint64 *)(msg->id.u64 + 1)))
    {
        MDV_LOGE("unbinn_get_table failed");
        return false;
    }

    return true;
}


bool mdv_msg_table_info_binn(mdv_msg_table_info const *msg, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_table_info failed");
        return false;
    }

    if (0
        || !binn_object_set_uint64(obj, "0", msg->id.u64[0])
        || !binn_object_set_uint64(obj, "1", msg->id.u64[1]))
    {
        binn_free(obj);
        MDV_LOGE("binn_table_info failed");
        return false;
    }

    return true;
}


bool mdv_msg_table_info_unbinn(binn const *obj, mdv_msg_table_info *msg)
{
    if (0
        || !binn_object_get_uint64((void*)obj, "0", (uint64 *)(msg->id.u64 + 0))
        || !binn_object_get_uint64((void*)obj, "1", (uint64 *)(msg->id.u64 + 1)))
    {
        MDV_LOGE("unbinn_table_info failed");
        return false;
    }

    return true;
}


bool mdv_msg_get_topology_binn(mdv_msg_get_topology const *msg, binn *obj)
{
    (void)msg;

    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_get_topology failed");
        return false;
    }

    return true;
}


bool mdv_msg_get_topology_unbinn(binn const *obj, mdv_msg_get_topology *msg)
{
    (void)msg;
    return true;
}


bool mdv_msg_topology_binn(mdv_msg_topology const *msg, binn *obj)
{
    return mdv_topology_serialize(msg->topology, obj);
}


mdv_topology * mdv_msg_topology_unbinn(binn const *obj)
{
    return mdv_topology_deserialize(obj);
}


bool mdv_msg_insert_into_binn(mdv_msg_insert_into const *msg, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("binn_insert_into failed");
        return false;
    }

    if (0
        || !binn_object_set_uint64(obj, "U0", msg->table.u64[0])
        || !binn_object_set_uint64(obj, "U1", msg->table.u64[1])
        || !binn_object_set_list(obj, "R", (void *)msg->rows))
    {
        binn_free(obj);
        MDV_LOGE("binn_insert_into failed");
        return false;
    }

    return true;
}


bool mdv_msg_insert_into_unbinn(binn const * obj, mdv_msg_insert_into *msg)
{
    if (0
        || !binn_object_get_uint64((void*)obj, "U0", (uint64 *)(msg->table.u64 + 0))
        || !binn_object_get_uint64((void*)obj, "U1", (uint64 *)(msg->table.u64 + 1))
        || !binn_object_get_list((void*)obj, "R", (void**)&msg->rows))
    {
        MDV_LOGE("unbinn_insert_into failed");
        return false;
    }

    return true;
}


bool mdv_msg_select_binn(mdv_msg_select const *msg, binn *obj)
{
    binn fields;

    if (!mdv_binn_bitset(msg->fields, &fields))
    {
        MDV_LOGE("mdv_msg_select_binn failed");
        return false;
    }

    if (!binn_create_object(obj))
    {
        MDV_LOGE("mdv_msg_select_binn failed");
        binn_free(&fields);
        return false;
    }

    if (0
        || !binn_object_set_uint64(obj, "T0", msg->table.u64[0])
        || !binn_object_set_uint64(obj, "T1", msg->table.u64[1])
        || !binn_object_set_list(obj,   "F", (void *)&fields)
        || !binn_object_set_str(obj,    "S", (char*)msg->filter))
    {
        MDV_LOGE("mdv_msg_select_binn failed");
        binn_free(obj);
        binn_free(&fields);
        return false;
    }

    binn_free(&fields);

    return true;
}


bool mdv_msg_select_unbinn(binn const * obj, mdv_msg_select *msg)
{
    binn *fields = 0;

    if (0
        || !binn_object_get_uint64((void*)obj, "T0", (uint64 *)(msg->table.u64 + 0))
        || !binn_object_get_uint64((void*)obj, "T1", (uint64 *)(msg->table.u64 + 1))
        || !binn_object_get_list((void*)obj,   "F", (void**)&fields)
        || !binn_object_get_str((void*)obj,    "S", (char**)&msg->filter))
    {
        MDV_LOGE("unbinn_insert_into failed");
        return false;
    }

    msg->fields = mdv_unbinn_bitset(fields);

    if (!msg->fields)
    {
        MDV_LOGE("unbinn_insert_into failed");
        return false;
    }

    return true;
}


void mdv_msg_select_free(mdv_msg_select *msg)
{
    if (msg->fields)
    {
        mdv_bitset_release(msg->fields);
        msg->fields = 0;
    }
}


bool mdv_msg_view_binn(mdv_msg_view const *msg, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("mdv_msg_view_binn failed");
        return false;
    }

    if (!binn_object_set_uint32(obj, "V", msg->id))
    {
        MDV_LOGE("mdv_msg_view_binn failed");
        binn_free(obj);
        return false;
    }

    return true;
}


bool mdv_msg_view_unbinn(binn const * obj, mdv_msg_view *msg)
{
    if (!binn_object_get_uint32((void*)obj, "V", &msg->id))
    {
        MDV_LOGE("mdv_msg_view_unbinn failed");
        return false;
    }

    return true;
}


bool mdv_msg_fetch_binn(mdv_msg_fetch const *msg, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("mdv_msg_fetch_binn failed");
        return false;
    }

    if (!binn_object_set_uint32(obj, "V", msg->id))
    {
        MDV_LOGE("mdv_msg_fetch_binn failed");
        binn_free(obj);
        return false;
    }

    return true;
}


bool mdv_msg_fetch_unbinn(binn const * obj, mdv_msg_fetch *msg)
{
    if (!binn_object_get_uint32((void*)obj, "V", &msg->id))
    {
        MDV_LOGE("mdv_msg_fetch_unbinn failed");
        return false;
    }

    return true;
}


bool mdv_msg_rowset_binn(mdv_msg_rowset const *msg, binn *obj)
{
    if (!binn_create_object(obj))
    {
        MDV_LOGE("mdv_msg_rowset_binn failed");
        return false;
    }

    if (0
        || !binn_object_set_list(obj, "R", (void *)msg->rows)
        || !binn_object_set_bool(obj, "E", msg->end))
    {
        MDV_LOGE("mdv_msg_rowset_binn failed");
        binn_free(obj);
        return false;
    }

    return true;
}


bool mdv_msg_rowset_unbinn(binn const * obj, mdv_msg_rowset *msg)
{
    BOOL end = 0;

    if(0
        || !binn_object_get_list((void*)obj, "R", (void**)&msg->rows)
        || !binn_object_get_bool((void*)obj, "E", &end))
    {
        MDV_LOGE("mdv_msg_rowset_unbinn failed");
        return false;
    }

    msg->end = end ? true : false;

    return true;
}

