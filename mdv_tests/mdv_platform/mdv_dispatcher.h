#pragma once
#include "../minunit.h"
#include <mdv_eventfd.h>
#include <mdv_dispatcher.h>
#include <mdv_threads.h>
#include <stdio.h>


static volatile int mdv_dispatcher_handler_1_state = 0;
static volatile int mdv_dispatcher_handler_2_state = 0;
static volatile int mdv_dispatcher_handler_3_state = 0;


static mdv_errno mdv_dispatcher_handler_1(mdv_msg const *msg, void *arg)
{
    (void)msg;
    (void)arg;
    mdv_dispatcher_handler_1_state = 1;
    return MDV_OK;
}

static mdv_errno mdv_dispatcher_handler_2(mdv_msg const *msg, void *arg)
{
    (void)msg;
    (void)arg;
    mdv_dispatcher_handler_2_state = 2;
    return MDV_OK;
}

static mdv_errno mdv_dispatcher_handler_3(mdv_msg const *msg, void *arg)
{
    (void)msg;
    (void)arg;
    mdv_dispatcher_handler_3_state = 3;
    return MDV_OK;
}


void *mdv_dispatcher_thread_test(void *arg)
{
    mdv_dispatcher *pd = (mdv_dispatcher *)arg;
    while(mdv_dispatcher_read(pd) != MDV_OK);
    return 0;
}


MU_TEST(platform_dispatcher)
{
    static const mdv_dispatcher_handler handlers[] =
    {
        { 1, &mdv_dispatcher_handler_1, 0 },
        { 2, &mdv_dispatcher_handler_2, 0 },
        { 3, &mdv_dispatcher_handler_3, 0 }
    };

    mdv_descriptor fd = mdv_eventfd();
    mu_check(fd != MDV_INVALID_DESCRIPTOR);

    mdv_msg msg =
    {
        .hdr =
        {
            .id = 0,
            .number = 0,
            .size = 0
        },
        .payload = 0
    };

    mdv_dispatcher *pd = mdv_dispatcher_create(fd);
    mu_check(pd);

    for(size_t i = 0; i < sizeof handlers / sizeof *handlers; ++i)
        mu_check(mdv_dispatcher_reg(pd, handlers + i) == MDV_OK);

    msg.hdr.id = 1;
    mu_check(mdv_dispatcher_post(pd, &msg) == MDV_OK);
    mu_check(mdv_dispatcher_read(pd) == MDV_OK);
    mu_check(mdv_dispatcher_handler_1_state = 1);

    msg.hdr.id = 2;
    mu_check(mdv_dispatcher_post(pd, &msg) == MDV_OK);
    mu_check(mdv_dispatcher_read(pd) == MDV_OK);
    mu_check(mdv_dispatcher_handler_2_state = 2);

    msg.hdr.id = 3;
    mu_check(mdv_dispatcher_post(pd, &msg) == MDV_OK);
    mu_check(mdv_dispatcher_read(pd) == MDV_OK);
    mu_check(mdv_dispatcher_handler_3_state = 3);


    mdv_thread_attrs attrs =
    {
        .stack_size = MDV_THREAD_STACK_SIZE
    };

    mdv_thread thread;
    mu_check(mdv_thread_create(&thread, &attrs, &mdv_dispatcher_thread_test, pd) == MDV_OK);

    msg.hdr.id = 42;
    mdv_msg resp = {};

    mu_check(mdv_dispatcher_send(pd, &msg, &resp, 1000) == MDV_OK);
    mu_check(resp.hdr.id == 42);

    mdv_free_msg(&resp);

    mdv_thread_join(thread);

    mdv_dispatcher_free(pd);

    mdv_eventfd_close(fd);
}

