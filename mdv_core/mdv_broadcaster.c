#include "mdv_broadcaster.h"
#include "event/mdv_types.h"
#include "event/mdv_broadcast.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <mdv_rollbacker.h>
#include <stdatomic.h>


struct mdv_broadcaster
{
    atomic_uint_fast32_t    rc;             ///< References counter
    mdv_ebus               *ebus;           ///< Events bus
};


static mdv_errno mdv_broadcaster_evt_handler(void *arg, mdv_event *event)
{
    mdv_broadcaster *broadcaster = arg;
    mdv_evt_broadcast *broadcast = (mdv_evt_broadcast *)event;

    mdv_errno err = MDV_FAILED;

    return err;
}


static const mdv_event_handler_type mdv_broadcaster_handlers[] =
{
    { MDV_EVT_BROADCAST, mdv_broadcaster_evt_handler },
};


mdv_broadcaster * mdv_broadcaster_create(mdv_ebus *ebus)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    mdv_broadcaster *broadcaster = mdv_alloc(sizeof(mdv_broadcaster), "broadcaster");

    if (!broadcaster)
    {
        MDV_LOGE("No memory for broadcaster");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, broadcaster, "broadcaster");

    atomic_init(&broadcaster->rc, 1);

    broadcaster->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, broadcaster->ebus);

    if (mdv_ebus_subscribe_all(broadcaster->ebus,
                               broadcaster,
                               mdv_broadcaster_handlers,
                               sizeof mdv_broadcaster_handlers / sizeof *mdv_broadcaster_handlers) != MDV_OK)
    {
        MDV_LOGE("Broadcaster subscription failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_free(rollbacker);

    return broadcaster;
}


mdv_broadcaster * mdv_broadcaster_retain(mdv_broadcaster *broadcaster)
{
    atomic_fetch_add_explicit(&broadcaster->rc, 1, memory_order_acquire);
    return broadcaster;
}


static void mdv_broadcaster_free(mdv_broadcaster *broadcaster)
{
    mdv_ebus_unsubscribe_all(broadcaster->ebus,
                             broadcaster,
                             mdv_broadcaster_handlers,
                             sizeof mdv_broadcaster_handlers / sizeof *mdv_broadcaster_handlers);

    mdv_ebus_release(broadcaster->ebus);
    mdv_free(broadcaster, "broadcaster");
}


uint32_t mdv_broadcaster_release(mdv_broadcaster *broadcaster)
{
    uint32_t rc = 0;

    if (broadcaster)
    {
        rc = atomic_fetch_sub_explicit(&broadcaster->rc, 1, memory_order_release) - 1;
        if (!rc)
            mdv_broadcaster_free(broadcaster);
    }

    return rc;
}
