#pragma once
#include "../minunit.h"
#include <mdv_chaman.h>
#include <mdv_threads.h>
#include <mdv_uuid.h>
#include <stdio.h>
#include <string.h>


static volatile int mdv_init_count = 0;
static volatile int mdv_close_count = 0;
static volatile int mdv_recv_size = 0;
static volatile mdv_descriptor fds[2];


typedef struct
{
    mdv_channel     base;
    uint32_t        rc;
    mdv_descriptor  fd;
    void           *userdata;
    mdv_channel_t   channel_type;
    mdv_channel_dir dir;
    mdv_uuid        id;
} mdv_test_channel;


static mdv_channel * mdv_test_channel_retain_impl(mdv_channel *channel)
{
    mdv_test_channel *test_channel = (mdv_test_channel *)channel;
    ++test_channel->rc;
    return channel;
}


static uint32_t mdv_test_channel_release_impl(mdv_channel *channel)
{
    uint32_t rc = 0;

    if (channel)
    {
        mdv_test_channel *test_channel = (mdv_test_channel *)channel;

        rc = --test_channel->rc;

        if (!rc)
        {
            ++mdv_close_count;
            mdv_free(channel, "test_channel") ;
        }
    }

    return rc;
}


static mdv_channel_t mdv_test_channel_type_impl(mdv_channel const *channel)
{
    mdv_test_channel *test_channel = (mdv_test_channel *)channel;
    return test_channel->channel_type;
}


static mdv_uuid const * mdv_test_channel_id_impl(mdv_channel const *channel)
{
    mdv_test_channel *test_channel = (mdv_test_channel *)channel;
    return &test_channel->id;
}


static mdv_errno mdv_test_channel_recv_impl(mdv_channel *channel)
{
    mdv_test_channel *test_channel = (mdv_test_channel *)channel;

    mdv_descriptor fd = test_channel->fd;

    char buffer[1024];

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


static mdv_errno mdv_test_channel_handshake_impl(mdv_descriptor fd, void *userdata)
{
    (void)userdata;
    char ch = 'a';
    size_t len = 1;
    return mdv_write(fd, &ch, &len);
}


static mdv_errno mdv_test_channel_accept_impl(mdv_descriptor  fd,
                                              void           *userdata,
                                              mdv_channel_t  *channel_type,
                                              mdv_uuid       *uuid)
{
    (void)userdata;
    char ch = 'a';
    size_t len = 1;
    mdv_read(fd, &ch, &len);
    *channel_type = 0;
    *uuid = mdv_uuid_generate();
    return MDV_OK;
}


static mdv_channel * mdv_test_channel_create_impl(mdv_descriptor    fd,
                                                  void             *userdata,
                                                  mdv_channel_t     channel_type,
                                                  mdv_channel_dir   dir,
                                                  mdv_uuid const   *channel_id)
{
    mdv_test_channel *channel = mdv_alloc(sizeof(mdv_test_channel), "test_channel");

    static const mdv_ichannel vtbl =
    {
        .retain = mdv_test_channel_retain_impl,
        .release = mdv_test_channel_release_impl,
        .type = mdv_test_channel_type_impl,
        .id = mdv_test_channel_id_impl,
        .recv = mdv_test_channel_recv_impl,
    };

    channel->base.vptr = &vtbl;

    channel->rc = 1;
    channel->fd = fd;
    channel->userdata = userdata;
    channel->channel_type = channel_type;
    channel->dir = dir;
    channel->id = *channel_id;

    fds[mdv_init_count++] = fd;

    return &channel->base;
}


MU_TEST(platform_chaman)
{
    mdv_chaman_config config =
    {
        .channel =
        {
            .retry_interval = 5,
            .keepidle       = 5,
            .keepcnt        = 10,
            .keepintvl      = 5,
            .handshake      = mdv_test_channel_handshake_impl,
            .accept         = mdv_test_channel_accept_impl,
            .create         = mdv_test_channel_create_impl,
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

    mdv_chaman *server = mdv_chaman_create(&config);
    mdv_errno err = mdv_chaman_listen(server, "tcp://localhost:55555");
    mu_check(server && err == MDV_OK);

    mdv_chaman *client = mdv_chaman_create(&config);
    mu_check(client);

    err = mdv_chaman_dial(client, "tcp://localhost:55555", 0);
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

