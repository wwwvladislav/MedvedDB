#include "mdv_table.h"
#include "mdv_types.h"
#include <mdv_alloc.h>


mdv_evt_create_table * mdv_evt_create_table_create(mdv_table_desc **table)
{
    static mdv_ievent vtbl =
    {
        .retain = (mdv_event_retain_fn)mdv_evt_create_table_retain,
        .release = (mdv_event_release_fn)mdv_evt_create_table_release
    };

    mdv_evt_create_table *event = (mdv_evt_create_table*)
                                mdv_event_create(
                                    MDV_EVT_CREATE_TABLE,
                                    sizeof(mdv_evt_create_table));

    if (event)
    {
        event->base.vptr = &vtbl;
        event->table = *table;
        *table = 0;
    }

    return event;
}


mdv_evt_create_table * mdv_evt_create_table_retain(mdv_evt_create_table *evt)
{
    return (mdv_evt_create_table*)mdv_event_retain(&evt->base);
}


uint32_t mdv_evt_create_table_release(mdv_evt_create_table *evt)
{
    mdv_table_desc *table = evt->table;

    uint32_t rc = mdv_event_release(&evt->base);

    if (!rc)
        mdv_free(table, "table");

    return rc;
}
