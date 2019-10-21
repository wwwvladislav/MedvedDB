#pragma once
#include "../minunit.h"
#include <mdv_ebus.h>
#include <stdatomic.h>


typedef struct
{
    mdv_event   base;
    atomic_int  rc;
} mdv_event_test;


static mdv_event * mdv_event_test_retain(mdv_event *base)
{
    mdv_event_test *event = (mdv_event_test *)base;
    atomic_fetch_add(&event->rc, 1);
    return base;
}


static uint32_t mdv_event_test_release(mdv_event *base)
{
    mdv_event_test *event = (mdv_event_test *)base;
    int rc = atomic_fetch_sub(&event->rc, 1) - 1;
    if (!rc)
        mdv_free(event, "test_event");
    return rc;
}


static mdv_errno mdv_event_test_handler(void *arg, mdv_event *event)
{
    atomic_int *counter = arg;
    atomic_fetch_add(counter, 1);
    (void)event;
    return MDV_OK;
}


MU_TEST(platform_ebus)
{
    mdv_ebus_config const config =
    {
        .threadpool =
        {
            .size = 2,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .event =
        {
            .queues_count = 4,
            .max_id = 1
        }
    };

    static mdv_ievent ievent_test =
    {
        .retain = mdv_event_test_retain,
        .release = mdv_event_test_release
    };

    mdv_ebus *ebus = mdv_ebus_create(&config);
    mu_check(ebus);

    mdv_event_test *event_0 = mdv_alloc(sizeof(mdv_event_test), "test_event");
    event_0->base.vptr = &ievent_test;
    event_0->base.type = 0;
    event_0->rc = 1;

    mdv_event_test *event_1 = mdv_alloc(sizeof(mdv_event_test), "test_event");
    event_1->base.vptr = &ievent_test;
    event_1->base.type = 1;
    event_1->rc = 1;

    atomic_int counter_0 = 0, counter_1 = 0;

    mu_check(mdv_ebus_subscribe(ebus,
                                0,
                                &counter_0,
                                mdv_event_test_handler) == MDV_OK);

    mu_check(mdv_ebus_subscribe(ebus,
                                1,
                                &counter_1,
                                mdv_event_test_handler) == MDV_OK);

    mu_check(mdv_ebus_subscribe(ebus,
                                0,
                                &counter_1,
                                mdv_event_test_handler) == MDV_OK);

    mdv_ebus_unsubscribe(ebus,
                         0,
                         &counter_1,
                         mdv_event_test_handler);

    for(int i = 0; i < 4; ++i)
    {
        mu_check(mdv_ebus_publish(ebus,
                                  (mdv_event *)event_0,
                                  MDV_EVT_DEFAULT) == MDV_OK);
        mu_check(mdv_ebus_publish(ebus,
                                  (mdv_event *)event_1,
                                  MDV_EVT_DEFAULT) == MDV_OK);
    }

    while(atomic_load(&counter_0) < 4);
    while(atomic_load(&counter_1) < 4);

    mu_check(mdv_ebus_publish(ebus,
                                (mdv_event *)event_0,
                                MDV_EVT_SYNC) == MDV_OK);
    mu_check(mdv_ebus_publish(ebus,
                                (mdv_event *)event_1,
                                MDV_EVT_SYNC) == MDV_OK);

    mu_check(atomic_load(&counter_0) == 5);
    mu_check(atomic_load(&counter_1) == 5);

    mdv_event_test_release((mdv_event *)event_0);
    mdv_event_test_release((mdv_event *)event_1);

    mdv_ebus_release(ebus);
}
