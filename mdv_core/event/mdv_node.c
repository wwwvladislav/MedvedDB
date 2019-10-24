#include "mdv_node.h"
#include "mdv_types.h"
#include <string.h>


mdv_evt_node_up * mdv_evt_node_up_create(mdv_uuid const *uuid, char const *addr)
{
    size_t const addr_len = strlen(addr);

    mdv_evt_node_up *event = (mdv_evt_node_up*)
                                mdv_event_create(
                                    MDV_EVT_NODE_UP,
                                    sizeof(mdv_evt_node_up) + addr_len);

    if (event)
    {
        event->node.uuid = *uuid;
        event->node.id = 0;
        memcpy(event->node.addr, addr, addr_len + 1);
    }

    return event;
}


mdv_evt_node_up * mdv_evt_node_up_retain(mdv_evt_node_up *evt)
{
    return (mdv_evt_node_up*)evt->base.vptr->retain(&evt->base);
}


uint32_t mdv_evt_node_up_release(mdv_evt_node_up *evt)
{
    return evt->base.vptr->release(&evt->base);
}


mdv_evt_node_registered * mdv_evt_node_registered_create(mdv_node const *node)
{
    size_t const addr_len = strlen(node->addr);

    mdv_evt_node_registered *event = (mdv_evt_node_registered*)
                                mdv_event_create(
                                    MDV_EVT_NODE_REGISTERED,
                                    sizeof(mdv_evt_node_registered) + addr_len);

    if (event)
    {
        event->node.uuid = node->uuid;
        event->node.id = node->id;
        memcpy(event->node.addr, node->addr, addr_len + 1);
    }

    return event;
}


mdv_evt_node_registered * mdv_evt_node_registered_retain(mdv_evt_node_registered *evt)
{
    return (mdv_evt_node_registered*)evt->base.vptr->retain(&evt->base);
}


uint32_t mdv_evt_node_registered_release(mdv_evt_node_registered *evt)
{
    return evt->base.vptr->release(&evt->base);
}
