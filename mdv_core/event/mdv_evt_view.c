#include "mdv_evt_view.h"
#include "mdv_evt_types.h"
#include <mdv_alloc.h>
#include <string.h>


mdv_evt_select * mdv_evt_select_create(mdv_uuid const  *session,
                                       uint16_t         request_id,
                                       mdv_uuid const  *table,
                                       mdv_bitset      *fields,
                                       char const      *filter)
{
    size_t const filter_len = strlen(filter);

    static mdv_ievent vtbl =
    {
        .retain = (mdv_event_retain_fn)mdv_evt_select_retain,
        .release = (mdv_event_release_fn)mdv_evt_select_release
    };

    mdv_evt_select *event = (mdv_evt_select*)
                                mdv_event_create(
                                    MDV_EVT_SELECT,
                                    sizeof(mdv_evt_select) + filter_len + 1);

    if (event)
    {
        char *data_space = (char*)(event + 1);

        memcpy(data_space, filter, filter_len + 1);

        event->base.vptr    = &vtbl;
        event->session      = *session;
        event->request_id   = request_id;
        event->table        = *table;
        event->fields       = mdv_bitset_retain(fields);
        event->filter       = data_space;
    }

    return event;
}


mdv_evt_select * mdv_evt_select_retain(mdv_evt_select *evt)
{
    return (mdv_evt_select*)mdv_event_retain(&evt->base);
}


uint32_t mdv_evt_select_release(mdv_evt_select *evt)
{
    mdv_bitset *fields = evt->fields;

    uint32_t rc = mdv_event_release(&evt->base);

    if (!rc)
        mdv_bitset_release(fields);

    return rc;
}


mdv_evt_view * mdv_evt_view_create(mdv_uuid const  *session,
                                   uint16_t         request_id,
                                   uint32_t         view_id)
{
    mdv_evt_view *event = (mdv_evt_view*)
                                mdv_event_create(
                                    MDV_EVT_VIEW,
                                    sizeof(mdv_evt_view));

    if (event)
    {
        event->session    = *session;
        event->request_id = request_id;
        event->view_id    = view_id;
    }

    return event;
}


mdv_evt_view * mdv_evt_view_retain(mdv_evt_view *evt)
{
    return (mdv_evt_view*)mdv_event_retain(&evt->base);
}


uint32_t mdv_evt_view_release(mdv_evt_view *evt)
{
    return mdv_event_release(&evt->base);
}


mdv_evt_view_fetch * mdv_evt_view_fetch_create(mdv_uuid const  *session,
                                               uint16_t         request_id,
                                               uint32_t         view_id)
{
    mdv_evt_view_fetch *event = (mdv_evt_view_fetch*)
                                mdv_event_create(
                                    MDV_EVT_VIEW_FETCH,
                                    sizeof(mdv_evt_view_fetch));

    if (event)
    {
        event->session    = *session;
        event->request_id = request_id;
        event->view_id    = view_id;
    }

    return event;
}


mdv_evt_view_fetch * mdv_evt_view_fetch_retain(mdv_evt_view_fetch *evt)
{
    return (mdv_evt_view_fetch*)mdv_event_retain(&evt->base);
}


uint32_t mdv_evt_view_fetch_release(mdv_evt_view_fetch *evt)
{
    return mdv_event_release(&evt->base);
}
