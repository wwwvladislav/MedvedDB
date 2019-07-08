#pragma once
#include "../minunit.h"
#include <mdv_chaman.h>
#include <mdv_threads.h>


MU_TEST(platform_chaman)
{
    mdv_chaman_config server_config =
    {
        .tp_size = 2,
        .uuid = mdv_uuid_generate(),
        .peer =
        {
            .reconnect_timeout = 5000,
            .keepalive_timeout = 5000
        }
    };

    mdv_chaman_config client_config =
    {
        .tp_size = 2,
        .uuid = mdv_uuid_generate(),
        .peer =
        {
            .reconnect_timeout = 5000,
            .keepalive_timeout = 5000
        }
    };

    mdv_chaman *server = mdv_chaman_create(&server_config);
    mdv_errno err = mdv_chaman_listen(server, mdv_str_static("tcp://localhost:55555"));
    mu_check(server && err == MDV_OK);

    mdv_chaman *client = mdv_chaman_create(&client_config);
    mu_check(client);

    err = mdv_chaman_connect(client, mdv_str_static("tcp://localhost:55555"));
    mu_check(err == MDV_OK);

    mdv_chaman_free(client);
    mdv_chaman_free(server);
}

