#include "mdv_broadcast.h"
#include "mdv_types.h"
#include <string.h>


mdv_evt_broadcast_post * mdv_evt_broadcast_post_create(
                                uint16_t        msg_id,
                                uint32_t        size,
                                void           *data,
                                mdv_hashmap    *notified,
                                mdv_hashmap    *peers)
{
    static mdv_ievent vtbl =
    {
        .retain = (mdv_event_retain_fn)mdv_evt_broadcast_post_retain,
        .release = (mdv_event_release_fn)mdv_evt_broadcast_post_release
    };

    mdv_evt_broadcast_post *event = (mdv_evt_broadcast_post*)
                                mdv_event_create(
                                    MDV_EVT_BROADCAST_POST,
                                    sizeof(mdv_evt_broadcast_post)
                                        + size);

    if (event)
    {
        event->base.vptr = &vtbl;

        event->msg_id   = msg_id;
        event->size     = size;
        event->data     = event + 1;
        event->notified = mdv_hashmap_retain(notified);
        event->peers    = mdv_hashmap_retain(peers);

        memcpy(event->data, data, size);
    }

    return event;
}


mdv_evt_broadcast_post * mdv_evt_broadcast_post_retain(mdv_evt_broadcast_post *evt)
{
    return (mdv_evt_broadcast_post*)mdv_event_retain(&evt->base);
}


uint32_t mdv_evt_broadcast_post_release(mdv_evt_broadcast_post *evt)
{
    mdv_hashmap *notified = evt->notified;
    mdv_hashmap *peers = evt->peers;

    uint32_t rc = mdv_event_release(&evt->base);

    if (!rc)
    {
        mdv_hashmap_release(notified);
        mdv_hashmap_release(peers);
    }

    return rc;
}


mdv_evt_broadcast * mdv_evt_broadcast_create(
                        mdv_uuid const *from,
                        uint16_t        msg_id,
                        uint32_t        size,
                        void           *data,
                        mdv_hashmap    *notified)
{
    static mdv_ievent vtbl =
    {
        .retain = (mdv_event_retain_fn)mdv_evt_broadcast_retain,
        .release = (mdv_event_release_fn)mdv_evt_broadcast_release
    };

    mdv_evt_broadcast *event = (mdv_evt_broadcast*)
                                mdv_event_create(
                                    MDV_EVT_BROADCAST,
                                    sizeof(mdv_evt_broadcast)
                                        + size);

    if (event)
    {
        event->base.vptr = &vtbl;

        event->from     = *from;
        event->msg_id   = msg_id;
        event->size     = size;
        event->data     = event + 1;
        event->notified = mdv_hashmap_retain(notified);

        memcpy(event->data, data, size);
    }

    return event;
}


mdv_evt_broadcast * mdv_evt_broadcast_retain(mdv_evt_broadcast *evt)
{
    return (mdv_evt_broadcast*)mdv_event_retain(&evt->base);
}


uint32_t mdv_evt_broadcast_release(mdv_evt_broadcast *evt)
{
    mdv_hashmap *notified = evt->notified;

    uint32_t rc = mdv_event_release(&evt->base);

    if (!rc)
        mdv_hashmap_release(notified);

    return rc;
}
