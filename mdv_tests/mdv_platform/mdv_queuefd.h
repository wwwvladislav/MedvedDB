#pragma once
#include "../minunit.h"
#include <mdv_queuefd.h>


MU_TEST(platform_queuefd)
{
    mdv_queuefd(int, 10) queue;
    mdv_queuefd_init(queue);

    mu_check(mdv_queuefd_ok(queue));

    int n = 0;
    int arr[] = { 1, 2, 3 };

    mu_check(mdv_queuefd_push(queue, n));
    mu_check(mdv_queuefd_push(queue, arr, 3));

    size_t event_count = mdv_queuefd_size(queue);
    mu_check(event_count == 4);

    int marr[4] = {};

    mu_check(mdv_queuefd_pop(queue, marr, event_count));

    mu_check(marr[0] == 0 &&
             marr[1] == 1 &&
             marr[2] == 2 &&
             marr[3] == 3);

    mdv_queuefd_free(queue);
}
