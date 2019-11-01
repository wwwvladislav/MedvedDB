#include "mdv_link.h"
#include "mdv_types.h"
#include <string.h>


mdv_evt_link_state * mdv_evt_link_state_create(
                                mdv_uuid const     *from,
                                mdv_toponode const *src,
                                mdv_toponode const *dst,
                                bool                connected)
{
    size_t const src_addr_size = strlen(src->addr) + 1;
    size_t const dst_addr_size = strlen(dst->addr) + 1;

    mdv_evt_link_state *event = (mdv_evt_link_state*)
                                mdv_event_create(
                                    MDV_EVT_LINK_STATE,
                                    sizeof(mdv_evt_link_state)
                                        + src_addr_size
                                        + dst_addr_size);

    if (event)
    {
        char *strings = (char*)(event + 1);

        event->from      = *from;
        event->src.uuid  = src->uuid;
        event->src.addr  = strings;
        event->dst.uuid  = dst->uuid;
        event->dst.addr  = strings + src_addr_size;
        event->connected = connected;

        memcpy(strings, src->addr, src_addr_size);
        memcpy(strings + src_addr_size, dst->addr, dst_addr_size);
    }

    return event;
}


mdv_evt_link_state * mdv_evt_link_state_retain(mdv_evt_link_state *evt)
{
    return (mdv_evt_link_state*)evt->base.vptr->retain(&evt->base);
}


uint32_t mdv_evt_link_state_release(mdv_evt_link_state *evt)
{
    return evt->base.vptr->release(&evt->base);
}

