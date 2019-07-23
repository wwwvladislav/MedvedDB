#pragma once
#include "../minunit.h"
#include <mdv_chaman.h>
#include <mdv_threads.h>
#include <stdio.h>


static volatile int mdv_init_count = 0;
static volatile int mdv_close_count = 0;
static volatile int mdv_recv_size = 0;
static volatile mdv_descriptor fds[2];


static mdv_errno mdv_channel_select(mdv_descriptor fd, uint8_t *type)
{
    size_t len = 1;
    char ch;
    mdv_read(fd, &ch, &len);
    *type = ch;
    return MDV_OK;
}


static void * mdv_channel_create(mdv_descriptor fd, mdv_string const *addr, void *userdata, uint8_t type, mdv_channel_dir dir)
{
    (void)fd;
    (void)userdata;
    (void)addr;
    (void)type;
    (void)dir;
    fds[mdv_init_count++] = fd;
    return fd;
}


static mdv_errno mdv_channel_recv(void *channel)
{
    mdv_descriptor fd = channel;

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


static void mdv_channel_close(void *channel)
{
    (void)channel;

    ++mdv_close_count;
}


MU_TEST(platform_chaman)
{
    mdv_chaman_config server_config =
    {
        .channel =
        {
            .keepidle   = 5,
            .keepcnt    = 10,
            .keepintvl  = 5,
            .select     = mdv_channel_select,
            .create     = mdv_channel_create,
            .recv       = mdv_channel_recv,
            .close      = mdv_channel_close
        },
        .threadpool =
        {
            .size = 2,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .userdata = 0
    };

    mdv_chaman_config client_config =
    {
        .channel =
        {
            .keepidle   = 5,
            .keepcnt    = 10,
            .keepintvl  = 5,
            .select     = mdv_channel_select,
            .create     = mdv_channel_create,
            .recv       = mdv_channel_recv,
            .close      = mdv_channel_close
        },
        .threadpool =
        {
            .size = 2,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .userdata = 0
    };

    mdv_chaman *server = mdv_chaman_create(&server_config);
    mdv_errno err = mdv_chaman_listen(server, mdv_str_static("tcp://localhost:55555"));
    mu_check(server && err == MDV_OK);

    mdv_chaman *client = mdv_chaman_create(&client_config);
    mu_check(client);

    err = mdv_chaman_connect(client, mdv_str_static("tcp://localhost:55555"), 0);
    mu_check(err == MDV_OK);

    while(mdv_init_count != 1)
        mdv_sleep(10);

    char ch = 'a';
    size_t len = 1;
    mu_check(mdv_write(fds[0], &ch, &len) == MDV_OK && len == 1);

    while(mdv_init_count != 2)
        mdv_sleep(10);

    char const msg[] = "The quick brown fox jumps over the lazy dog";
    len = sizeof msg;

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

