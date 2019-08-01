#pragma once
#include "../minunit.h"
#include <mdv_jobber.h>


MU_TEST(platform_jobber)
{
    mdv_jobber_config const config =
    {
        .threadpool =
        {
            .size = 2,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .queue =
        {
            .count = 4
        }
    };

    mdv_jobber *jobber = mdv_jobber_create(&config);

    mu_check(jobber);


    mdv_jobber_free(jobber);
}

