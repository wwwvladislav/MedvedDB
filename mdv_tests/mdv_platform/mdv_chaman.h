#pragma once
#include "../minunit.h"
#include <mdv_chaman.h>
#include <mdv_threads.h>
#include <stdio.h>


static volatile int mdv_init_count = 0;
static volatile int mdv_close_count = 0;
static volatile int mdv_recv_size = 0;
static volatile mdv_descriptor fds[2];


static void mdv_channel_init(void *userdata, void *context, mdv_descriptor fd, mdv_string const *addr)
{
    (void)fd;
    (void)context;
    (void)userdata;
    (void)addr;
    fds[mdv_init_count++] = fd;
}


static mdv_errno mdv_channel_recv(void *userdata, void *context, mdv_descriptor fd)
{
    (void)context;
    (void)userdata;

    static _Thread_local char buffer[1024];

    size_t len = sizeof(buffer) - 1;

    mdv_errno err = mdv_read(fd, buffer, &len);

    while(err == MDV_OK)
    {
        mdv_recv_size += len;

        len = sizeof(buffer) - 1;
        err = mdv_read(fd, buffer, &len);
    }

    return err;
}


static void mdv_channel_close(void *userdata, void *context)
{
    (void)context;
    (void)userdata;
    ++mdv_close_count;
}


MU_TEST(platform_chaman)
{
    mdv_chaman_config server_config =
    {
        .peer =
        {
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
        .userdata = 0,
        .channel =
        {
            .context =
            {
                .size = 4,
                .guardsize = 4
            },
            .init = mdv_channel_init,
            .recv = mdv_channel_recv,
            .close = mdv_channel_close
        }
    };

    mdv_chaman_config client_config =
    {
        .peer =
        {
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
        .userdata = 0,
        .channel =
        {
            .context =
            {
                .size = 4,
                .guardsize = 4
            },
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

    while(mdv_init_count != 2)
        mdv_sleep(10);

    char const msg[] = "The quick brown fox jumps over the lazy dog";
    size_t len = sizeof msg;

    mu_check(mdv_write(fds[0], msg, &len) == MDV_OK && len == sizeof msg);
    mu_check(mdv_write(fds[1], msg, &len) == MDV_OK && len == sizeof msg);

    while(mdv_recv_size != 2 * sizeof msg)
        mdv_sleep(10);

    mdv_socket_shutdown(fds[0], MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);

    while(mdv_close_count != 2)
        mdv_sleep(10);

    mdv_chaman_free(client);
    mdv_chaman_free(server);
}

