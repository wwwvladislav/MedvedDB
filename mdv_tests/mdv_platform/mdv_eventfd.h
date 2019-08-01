#pragma once
#include "../minunit.h"
#include <mdv_eventfd.h>
#include <stdint.h>


MU_TEST(platform_eventfd)
{
    mdv_descriptor fd = mdv_eventfd(true);

    mu_check(fd != MDV_INVALID_DESCRIPTOR);

    uint64_t data[] = { 1, 2, 3 };
    size_t len;
    mdv_errno err;

    for (size_t written = 0; written < sizeof data;)
    {
        len = sizeof data - written;
        err = mdv_write(fd, (uint8_t *)data + written, &len);
        mu_check(err == MDV_OK);
        written += len;
    }

    uint64_t counter = 0, sum = 0;

    len = sizeof counter;

    while(mdv_read(fd, &counter, &len) == MDV_OK)
        sum += counter;

    mu_check(sum == data[0] + data[1] + data[2]);

    len = sizeof counter;
    err = mdv_read(fd, &counter, &len);
    mu_check(err == MDV_EAGAIN);

    mdv_eventfd_close(fd);
}

