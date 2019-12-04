#include "mdv_evt_trlog.h"
#include "mdv_evt_types.h"


mdv_evt_trlog * mdv_evt_trlog_create(mdv_uuid const *uuid, bool create)
{
    static mdv_ievent vtbl =
    {
        .retain = (mdv_event_retain_fn)mdv_evt_trlog_retain,
        .release = (mdv_event_release_fn)mdv_evt_trlog_release
    };

    mdv_evt_trlog *event = (mdv_evt_trlog*)
                                mdv_event_create(
                                    MDV_EVT_TRLOG_GET,
                                    sizeof(mdv_evt_trlog));

    if (event)
    {
        event->base.vptr = &vtbl;
        event->uuid      = *uuid;
        event->trlog     = 0;
        event->create    = create;
    }

    return event;
}


mdv_evt_trlog * mdv_evt_trlog_retain(mdv_evt_trlog *evt)
{
    return (mdv_evt_trlog*)mdv_event_retain(&evt->base);
}


uint32_t mdv_evt_trlog_release(mdv_evt_trlog *evt)
{
    mdv_trlog *trlog = evt->trlog;

    uint32_t rc = mdv_event_release(&evt->base);

    if (!rc)
        mdv_trlog_release(trlog);

    return rc;
}


mdv_evt_trlog_changed * mdv_evt_trlog_changed_create(mdv_uuid const *trlog)
{
    mdv_evt_trlog_changed *event = (mdv_evt_trlog_changed*)
                                mdv_event_create(
                                    MDV_EVT_TRLOG_CHANGED,
                                    sizeof(mdv_evt_trlog_changed));

    if (event)
        event->trlog = *trlog;

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


mdv_evt_trlog_data * mdv_evt_trlog_data_create(mdv_uuid const *trlog, mdv_uuid const *from, mdv_uuid const *to, mdv_list *rows, uint32_t count)
{
    static mdv_ievent vtbl =
    {
        .retain = (mdv_event_retain_fn)mdv_evt_trlog_data_retain,
        .release = (mdv_event_release_fn)mdv_evt_trlog_data_release
    };

    mdv_evt_trlog_data *event = (mdv_evt_trlog_data*)
                                mdv_event_create(
                                    MDV_EVT_TRLOG_DATA,
                                    sizeof(mdv_evt_trlog_data));

    if (event)
    {
        event->base.vptr = &vtbl;
        event->from      = *from;
        event->to        = *to;
        event->trlog     = *trlog;
        event->count     = count;
        event->rows      = mdv_list_move(rows);
    }

    return event;
}


mdv_evt_trlog_data * mdv_evt_trlog_data_retain(mdv_evt_trlog_data *evt)
{
    return (mdv_evt_trlog_data*)mdv_event_retain(&evt->base);
}


uint32_t mdv_evt_trlog_data_release(mdv_evt_trlog_data *evt)
{
    mdv_list rows = evt->rows;

    uint32_t rc = mdv_event_release(&evt->base);

    if (!rc)
        mdv_list_clear(&rows);

    return rc;
}
