#include "mdv_evt_status.h"
#include "mdv_evt_types.h"


mdv_evt_status * mdv_evt_status_create(
                                mdv_uuid const *session,
                                uint16_t        request_id,
                                int             err,
                                char const     *message)
{
    mdv_evt_status *event = (mdv_evt_status*)
                                mdv_event_create(
                                    MDV_EVT_STATUS,
                                    sizeof(mdv_evt_status));

    if (event)
    {
        event->session    = *session;
        event->request_id = request_id;
        event->err        = err;
        event->message    = message;
    }

    return event;
}


mdv_evt_status * mdv_evt_status_retain(mdv_evt_status *evt)
{
    return (mdv_evt_status*)evt->base.vptr->retain(&evt->base);
}


uint32_t mdv_evt_status_release(mdv_evt_status *evt)
{
    return evt->base.vptr->release(&evt->base);
}
