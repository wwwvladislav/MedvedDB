#pragma once
#include "../minunit.h"
#include <mdv_chaman.h>
#include <mdv_threads.h>


MU_TEST(platform_chaman)
{
    mdv_chaman *chaman = mdv_chaman_create(2);

    mu_check(chaman);

    mdv_errno err = mdv_chaman_listen(chaman, mdv_str_static("tcp://localhost:55555"));

    mdv_sleep(5 * 60 * 1000);

    mdv_chaman_free(chaman);
}

