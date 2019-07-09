#pragma once
#include "../minunit.h"
#include <mdv_threadpool.h>
#include <mdv_eventfd.h>
#include <mdv_threads.h>
#include <mdv_log.h>
#include <stdint.h>
#include <stdatomic.h>


static atomic_int_fast32_t test_fd_data_sum = 0;
static atomic_int_fast32_t test_fd_events_num = 0;


static void test_fd_handler(mdv_descriptor fd, uint32_t events, void *data)
{
    (void)events;
    (void)data;

    uint64_t counter = 0;
    size_t len = sizeof counter;
    mdv_errno err = mdv_read(fd, &counter, &len);
    (void)err;

    atomic_fetch_add_explicit(&test_fd_data_sum, counter, memory_order_release);
    atomic_fetch_add_explicit(&test_fd_events_num, 1, memory_order_release);
}


MU_TEST(platform_threadpool)
{
    mdv_threadpool_config config =
    {
        .size = 4,
        .thread_attrs =
        {
            .stack_size = MDV_THREAD_STACK_SIZE
        }
    };


    mdv_threadpool *tp = mdv_threadpool_create(&config);
    mu_check(tp);

    mdv_descriptor fd[] = { mdv_eventfd(), mdv_eventfd(), mdv_eventfd() };

    mu_check(fd[0] != MDV_INVALID_DESCRIPTOR);
    mu_check(fd[1] != MDV_INVALID_DESCRIPTOR);
    mu_check(fd[2] != MDV_INVALID_DESCRIPTOR);

    mdv_threadpool_task_base tasks[] =
    {
        {
            .fd = fd[0],
            .fn = test_fd_handler,
            .context_size = 1
        },
        {
            .fd = fd[1],
            .fn = test_fd_handler,
            .context_size = 1
        },
        {
            .fd = fd[2],
            .fn = test_fd_handler,
            .context_size = 1
        }
    };

    mu_check(mdv_threadpool_add(tp, MDV_EPOLLET | MDV_EPOLLEXCLUSIVE | MDV_EPOLLIN | MDV_EPOLLERR, tasks + 0) == MDV_OK);
    mu_check(mdv_threadpool_add(tp, MDV_EPOLLET | MDV_EPOLLEXCLUSIVE | MDV_EPOLLIN | MDV_EPOLLERR, tasks + 1) == MDV_OK);
    mu_check(mdv_threadpool_add(tp, MDV_EPOLLET | MDV_EPOLLEXCLUSIVE | MDV_EPOLLIN | MDV_EPOLLERR, tasks + 2) == MDV_OK);

    mu_check(atomic_load_explicit(&test_fd_data_sum, memory_order_relaxed) == 0);

    uint64_t fddata = 42;
    size_t len = sizeof(fddata);

    mu_check(mdv_write(fd[0], &fddata, &len) == MDV_OK);
    mu_check(mdv_write(fd[1], &fddata, &len) == MDV_OK);
    mu_check(mdv_write(fd[2], &fddata, &len) == MDV_OK);

    while(atomic_load_explicit(&test_fd_events_num, memory_order_acquire) < 3);

    mu_check(atomic_load_explicit(&test_fd_data_sum, memory_order_acquire) == (uint32_t)(3 * fddata));
    mu_check(atomic_load_explicit(&test_fd_events_num, memory_order_acquire) == 3);

    mdv_eventfd_close(fd[0]);
    mdv_eventfd_close(fd[1]);
    mdv_eventfd_close(fd[2]);

    mdv_threadpool_free(tp);
}

