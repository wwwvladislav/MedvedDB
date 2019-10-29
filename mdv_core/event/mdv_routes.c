#include "mdv_routes.h"
#include "mdv_types.h"


mdv_evt_routes_changed * mdv_evt_routes_changed_create(mdv_hashmap *routes)
{
    static mdv_ievent vtbl =
    {
        .retain = (mdv_event_retain_fn)mdv_evt_routes_changed_retain,
        .release = (mdv_event_release_fn)mdv_evt_routes_changed_release
    };

    mdv_evt_routes_changed *event = (mdv_evt_routes_changed*)
                                mdv_event_create(
                                    MDV_EVT_ROUTES_CHANGED,
                                    sizeof(mdv_evt_routes_changed));

    if (event)
    {
        event->base.vptr = &vtbl;
        event->routes = mdv_hashmap_retain(routes);
    }

    return event;
}


mdv_evt_routes_changed * mdv_evt_routes_changed_retain(mdv_evt_routes_changed *evt)
{
    return (mdv_evt_routes_changed*)mdv_event_retain(&evt->base);
}


uint32_t mdv_evt_routes_changed_release(mdv_evt_routes_changed *evt)
{
    mdv_hashmap *routes = evt->routes;

    uint32_t rc = mdv_event_release(&evt->base);

    if (!rc)
        mdv_hashmap_release(routes);

    return rc;
}

