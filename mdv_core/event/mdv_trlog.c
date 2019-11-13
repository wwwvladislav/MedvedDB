#include "mdv_trlog.h"
#include "mdv_types.h"


mdv_evt_trlog_changed * mdv_evt_trlog_changed_create()
{
    static mdv_ievent vtbl =
    {
        .retain = (mdv_event_retain_fn)mdv_evt_trlog_changed_retain,
        .release = (mdv_event_release_fn)mdv_evt_trlog_changed_release
    };

    static mdv_evt_trlog_changed evt =
    {
        .base =
        {
            .vptr = &vtbl,
            .type = MDV_EVT_TRLOG_CHANGED,
            .rc = 1
        }
    };

    return &evt;
}


mdv_evt_trlog_changed * mdv_evt_trlog_changed_retain(mdv_evt_trlog_changed *evt)
{
    return evt;
}


uint32_t mdv_evt_trlog_changed_release(mdv_evt_trlog_changed *evt)
{
    return 1u;
}


mdv_evt_trlog_apply * mdv_evt_trlog_apply_create(mdv_uuid const *uuid)
{
    mdv_evt_trlog_apply *event = (mdv_evt_trlog_apply*)
                                mdv_event_create(
                                    MDV_EVT_TRLOG_APPLY,
                                    sizeof(mdv_evt_trlog_apply));

    if (event)
        event->uuid = *uuid;

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
