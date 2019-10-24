#include "mdv_link.h"
#include "mdv_types.h"


mdv_evt_link_state * mdv_evt_link_state_create(
                                mdv_uuid const *from,
                                mdv_uuid const *src,
                                mdv_uuid const *dst,
                                bool connected)
{
    mdv_evt_link_state *event = (mdv_evt_link_state*)
                                mdv_event_create(
                                    MDV_EVT_LINK_STATE,
                                    sizeof(mdv_evt_link_state));

    if (event)
    {
        event->from = *from;
        event->src = *src;
        event->dst = *dst;
        event->connected = connected;
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


mdv_evt_link_state_broadcast * mdv_evt_link_state_broadcast_create(
                                                mdv_vector     *routes,
                                                mdv_uuid const *from,
                                                mdv_uuid const *src,
                                                mdv_uuid const *dst,
                                                bool            connected)
{
    static mdv_ievent vtbl =
    {
        .retain = (mdv_event_retain_fn)mdv_evt_link_state_broadcast_retain,
        .release = (mdv_event_release_fn)mdv_evt_link_state_broadcast_release
    };

    mdv_evt_link_state_broadcast *event = (mdv_evt_link_state_broadcast*)
                                mdv_event_create(
                                    MDV_EVT_LINK_STATE_BROADCAST,
                                    sizeof(mdv_evt_link_state_broadcast));

    if (event)
    {
        event->base.vptr = &vtbl;
        event->routes = mdv_vector_retain(routes);
        event->from = *from;
        event->src = *src;
        event->dst = *dst;
        event->connected = connected;
    }

    return event;
}


mdv_evt_link_state_broadcast * mdv_evt_link_state_broadcast_retain(mdv_evt_link_state_broadcast *evt)
{
    return (mdv_evt_link_state_broadcast*)mdv_event_retain(&evt->base);
}


uint32_t mdv_evt_link_state_broadcast_release(mdv_evt_link_state_broadcast *evt)
{
    mdv_vector *routes = evt->routes;

    uint32_t rc = mdv_event_release(&evt->base);

    if (!rc)
        mdv_vector_release(routes);

    return rc;
}
