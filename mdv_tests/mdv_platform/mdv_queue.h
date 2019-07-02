#pragma once
#include "../minunit.h"
#include <mdv_queue.h>


MU_TEST(platform_queue)
{
    mdv_queue(int, 3) queue;
    mdv_queue_clear(queue);

    mu_check(mdv_queue_empty(queue));
    mu_check(mdv_queue_capacity(queue) == 3);
    mu_check(mdv_queue_size(queue) == 0);
    mu_check(mdv_queue_free_space(queue) == 3);

    int n = 0, m = 0;

    mu_check(mdv_queue_push(queue, n)); n++;
    mu_check(mdv_queue_push(queue, n)); n++;
    mu_check(mdv_queue_push(queue, n)); n++;
    mu_check(!mdv_queue_push(queue, n));

    mu_check(mdv_queue_size(queue) == 3);

    mu_check(mdv_queue_pop(queue, m) && m == 0);
    mu_check(mdv_queue_pop(queue, m) && m == 1);

    mu_check(mdv_queue_size(queue) == 1);

    mu_check(mdv_queue_push(queue, n)); n++;
    mu_check(mdv_queue_push(queue, n)); n++;
    mu_check(!mdv_queue_push(queue, n));

    mu_check(mdv_queue_size(queue) == 3);

    mu_check(mdv_queue_pop(queue, m) && m == 2);
    mu_check(mdv_queue_pop(queue, m) && m == 3);
    mu_check(mdv_queue_pop(queue, m) && m == 4);

    mu_check(mdv_queue_empty(queue));

    int arr_0[] = { n++, n++ };
    mu_check(mdv_queue_push(queue, arr_0, 2));

    mu_check(mdv_queue_pop(queue, m) && m == 5);
    mu_check(mdv_queue_pop(queue, m) && m == 6);

    mu_check(mdv_queue_empty(queue));

    int arr_1[] = { n++, n++ };
    mu_check(mdv_queue_push(queue, arr_1, 2));

    int arr_2[2];
    mu_check(mdv_queue_pop_multiple(queue, arr_2, 2) && arr_2[0] == 7 && arr_2[1] == 8);
}
