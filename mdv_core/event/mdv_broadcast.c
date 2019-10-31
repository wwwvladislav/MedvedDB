#include "mdv_broadcast.h"
#include "mdv_types.h"
#include <string.h>


mdv_evt_broadcast * mdv_evt_broadcast_start(
                        uint16_t        msg_id,
                        uint32_t        size,
                        void           *data)
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

        event->msg_id = msg_id;
        event->size = size;
        event->data = event + 1;

        event->notified = _mdv_hashmap_create(4,
                                              0,
                                              sizeof(mdv_uuid),
                                              (mdv_hash_fn)mdv_uuid_hash,
                                              (mdv_key_cmp_fn)mdv_uuid_cmp);

        if (event->notified)
        {
            memcpy(event->data, data, size);
        }
        else
        {
            mdv_event_release(&event->base);
            event = 0;
        }
    }

    return event;
}


mdv_evt_broadcast * mdv_evt_broadcast_create(
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

        event->msg_id = msg_id;
        event->size = size;
        event->data = event + 1;
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
