#pragma once
#include "../minunit.h"
#include <mdv_chaman.h>
#include <mdv_threads.h>


static void mdv_channel_init(void *context, mdv_descriptor fd)
{}


static void mdv_channel_recv(void *context, mdv_descriptor fd)
{}


static void mdv_channel_close(void *context)
{}


MU_TEST(platform_chaman)
{
    mdv_chaman_config server_config =
    {
        .peer =
        {
            .reconnect_timeout = 5,
            .keepidle          = 5,
            .keepcnt           = 10,
            .keepintvl         = 5
        },
        .threadpool =
        {
            .size = 2,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .channel =
        {
            .init = mdv_channel_init,
            .recv = mdv_channel_recv,
            .close = mdv_channel_close
        }
    };

    mdv_chaman_config client_config =
    {
        .peer =
        {
            .reconnect_timeout = 5,
            .keepidle          = 5,
            .keepcnt           = 10,
            .keepintvl         = 5
        },
        .threadpool =
        {
            .size = 2,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .channel =
        {
            .init = mdv_channel_init,
            .recv = mdv_channel_recv,
            .close = mdv_channel_close
        }
    };

    mdv_chaman *server = mdv_chaman_create(&server_config);
    mdv_errno err = mdv_chaman_listen(server, mdv_str_static("tcp://localhost:55555"));
    mu_check(server && err == MDV_OK);

    mdv_chaman *client = mdv_chaman_create(&client_config);
    mu_check(client);

    err = mdv_chaman_connect(client, mdv_str_static("tcp://localhost:55555"));
    mu_check(err == MDV_OK);

    mdv_sleep(5 * 60 * 1000);

    mdv_chaman_free(client);
    mdv_chaman_free(server);
}

