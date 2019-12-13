#include "mdv_evt_rowdata.h"
#include "mdv_evt_types.h"
#include <mdv_alloc.h>


mdv_evt_rowdata_ins_req * mdv_evt_rowdata_ins_req_create(mdv_uuid const *table_id, binn *rows)
{
    mdv_evt_rowdata_ins_req *event = (mdv_evt_rowdata_ins_req*)
                                mdv_event_create(
                                    MDV_EVT_ROWDATA_INSERT,
                                    sizeof(mdv_evt_rowdata_ins_req));

    if (event)
    {
        event->table_id = *table_id;
        event->rows = rows;
    }

    return event;
}


mdv_evt_rowdata_ins_req * mdv_evt_rowdata_ins_req_retain(mdv_evt_rowdata_ins_req *evt)
{
    return (mdv_evt_rowdata_ins_req*)evt->base.vptr->retain(&evt->base);
}


uint32_t mdv_evt_rowdata_ins_req_release(mdv_evt_rowdata_ins_req *evt)
{
    return evt->base.vptr->release(&evt->base);
}


mdv_evt_rowdata_fetch * mdv_evt_rowdata_fetch_create(mdv_uuid const  *session,
                                                     uint16_t         request_id,
                                                     mdv_uuid const  *table,
                                                     bool             first,
                                                     mdv_objid const *rowid,
                                                     uint32_t         count)
{
    mdv_evt_rowdata_fetch *event = (mdv_evt_rowdata_fetch*)
                                mdv_event_create(
                                    MDV_EVT_ROWDATA_FETCH,
                                    sizeof(mdv_evt_rowdata_fetch));

    if (event)
    {
        event->session    = *session;
        event->request_id = request_id;
        event->table      = *table;
        event->first      = first;
        event->rowid      = *rowid;
        event->count      = count;
    }

    return event;
}


mdv_evt_rowdata_fetch * mdv_evt_rowdata_fetch_retain(mdv_evt_rowdata_fetch *evt)
{
    return (mdv_evt_rowdata_fetch*)evt->base.vptr->retain(&evt->base);
}


uint32_t mdv_evt_rowdata_fetch_release(mdv_evt_rowdata_fetch *evt)
{
    return evt->base.vptr->release(&evt->base);
}
