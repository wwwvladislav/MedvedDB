#include "mdv_evt_tables.h"
#include "mdv_evt_types.h"
#include <mdv_alloc.h>


mdv_evt_tables * mdv_evt_tables_create()
{
    static mdv_ievent vtbl =
    {
        .retain = (mdv_event_retain_fn)mdv_evt_tables_retain,
        .release = (mdv_event_release_fn)mdv_evt_tables_release
    };

    mdv_evt_tables *event = (mdv_evt_tables*)
                                mdv_event_create(
                                    MDV_EVT_TABLES_GET,
                                    sizeof(mdv_evt_tables));

    if (event)
    {
        event->base.vptr = &vtbl;
        event->tables    = 0;
    }

    return event;

}


mdv_evt_tables * mdv_evt_tables_retain(mdv_evt_tables *evt)
{
    return (mdv_evt_tables*)mdv_event_retain(&evt->base);
}


uint32_t mdv_evt_tables_release(mdv_evt_tables *evt)
{
    mdv_tables *tables = evt->tables;

    uint32_t rc = mdv_event_release(&evt->base);

    if (!rc)
        mdv_tables_release(tables);

    return rc;
}
