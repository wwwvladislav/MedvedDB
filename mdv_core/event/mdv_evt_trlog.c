#include "mdv_evt_trlog.h"
#include "mdv_evt_types.h"


mdv_evt_trlog_changed * mdv_evt_trlog_changed_create(mdv_uuid const *uuid)
{
    mdv_evt_trlog_changed *event = (mdv_evt_trlog_changed*)
                                mdv_event_create(
                                    MDV_EVT_TRLOG_CHANGED,
                                    sizeof(mdv_evt_trlog_changed));

    if (event)
        event->uuid = *uuid;

    return event;
}


mdv_evt_trlog_changed * mdv_evt_trlog_changed_retain(mdv_evt_trlog_changed *evt)
{
    return (mdv_evt_trlog_changed*)evt->base.vptr->retain(&evt->base);
}


uint32_t mdv_evt_trlog_changed_release(mdv_evt_trlog_changed *evt)
{
    return evt->base.vptr->release(&evt->base);
}


mdv_evt_trlog_apply * mdv_evt_trlog_apply_create(mdv_uuid const *trlog)
{
    mdv_evt_trlog_apply *event = (mdv_evt_trlog_apply*)
                                mdv_event_create(
                                    MDV_EVT_TRLOG_APPLY,
                                    sizeof(mdv_evt_trlog_apply));

    if (event)
        event->trlog = *trlog;

    return event;
}


mdv_evt_trlog_apply * mdv_evt_trlog_apply_retain(mdv_evt_trlog_apply *evt)
{
    return (mdv_evt_trlog_apply*)evt->base.vptr->retain(&evt->base);
}


uint32_t mdv_evt_trlog_apply_release(mdv_evt_trlog_apply *evt)
{
    return evt->base.vptr->release(&evt->base);
}


mdv_evt_trlog_sync * mdv_evt_trlog_sync_create(mdv_uuid const *trlog, mdv_uuid const *from, mdv_uuid const *to)
{
    mdv_evt_trlog_sync *event = (mdv_evt_trlog_sync*)
                                mdv_event_create(
                                    MDV_EVT_TRLOG_SYNC,
                                    sizeof(mdv_evt_trlog_sync));

    if (event)
    {
        event->from  = *from;
        event->to    = *to;
        event->trlog = *trlog;
    }

    return event;
}


mdv_evt_trlog_sync * mdv_evt_trlog_sync_retain(mdv_evt_trlog_sync *evt)
{
    return (mdv_evt_trlog_sync*)evt->base.vptr->retain(&evt->base);
}


uint32_t mdv_evt_trlog_sync_release(mdv_evt_trlog_sync *evt)
{
    return evt->base.vptr->release(&evt->base);
}


mdv_evt_trlog_state * mdv_evt_trlog_state_create(mdv_uuid const *trlog, mdv_uuid const *from, mdv_uuid const *to, uint64_t top)
{
    mdv_evt_trlog_state *event = (mdv_evt_trlog_state*)
                                mdv_event_create(
                                    MDV_EVT_TRLOG_STATE,
                                    sizeof(mdv_evt_trlog_state));

    if (event)
    {
        event->from  = *from;
        event->to    = *to;
        event->trlog = *trlog;
        event->top   = top;
    }

    return event;
}


mdv_evt_trlog_state * mdv_evt_trlog_state_retain(mdv_evt_trlog_state *evt)
{
    return (mdv_evt_trlog_state*)evt->base.vptr->retain(&evt->base);
}


uint32_t mdv_evt_trlog_state_release(mdv_evt_trlog_state *evt)
{
    return evt->base.vptr->release(&evt->base);
}
