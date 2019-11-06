#include "mdv_topology.h"
#include "mdv_types.h"
#include <mdv_log.h>


mdv_evt_topology * mdv_evt_topology_create(mdv_topology *topology)
{
    static mdv_ievent vtbl =
    {
        .retain = (mdv_event_retain_fn)mdv_evt_topology_retain,
        .release = (mdv_event_release_fn)mdv_evt_topology_release
    };

    mdv_evt_topology *event = (mdv_evt_topology*)
                                mdv_event_create(
                                    MDV_EVT_TOPOLOGY,
                                    sizeof(mdv_evt_topology));

    if (event)
    {
        event->base.vptr = &vtbl;
        event->topology  = mdv_topology_retain(topology);
    }

    return event;
}


mdv_evt_topology * mdv_evt_topology_retain(mdv_evt_topology *evt)
{
    return (mdv_evt_topology*)mdv_event_retain(&evt->base);
}


uint32_t mdv_evt_topology_release(mdv_evt_topology *evt)
{
    mdv_topology *topology = evt->topology;

    uint32_t rc = mdv_event_release(&evt->base);

    if (!rc)
        mdv_topology_release(topology);

    return rc;
}


mdv_evt_topology_sync * mdv_evt_topology_sync_create(mdv_uuid const *from,
                                                     mdv_uuid const *to,
                                                     mdv_topology   *topology)
{
    static mdv_ievent vtbl =
    {
        .retain = (mdv_event_retain_fn)mdv_evt_topology_sync_retain,
        .release = (mdv_event_release_fn)mdv_evt_topology_sync_release
    };

    mdv_evt_topology_sync *event = (mdv_evt_topology_sync*)
                                mdv_event_create(
                                    MDV_EVT_TOPOLOGY_SYNC,
                                    sizeof(mdv_evt_topology_sync));

    if (event)
    {
        event->base.vptr = &vtbl;
        event->from      = *from;
        event->to        = *to;
        event->topology  = mdv_topology_retain(topology);
    }

    return event;
}


mdv_evt_topology_sync * mdv_evt_topology_sync_retain(mdv_evt_topology_sync *evt)
{
    return (mdv_evt_topology_sync*)mdv_event_retain(&evt->base);
}


uint32_t mdv_evt_topology_sync_release(mdv_evt_topology_sync *evt)
{
    mdv_topology *topology = evt->topology;

    uint32_t rc = mdv_event_release(&evt->base);

    if (!rc)
        mdv_topology_release(topology);

    return rc;
}
