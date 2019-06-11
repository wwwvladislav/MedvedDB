#pragma once
#include "../minunit.h"
#include <mdv_filesystem.h>

MU_TEST(platform_mkdir)
{
    mu_check(mdv_mkdir("./ololo/alala/atata"));
    mu_check(mdv_mkdir("./ololo/alala/azaza"));
    mu_check(mdv_rmdir("./ololo"));
}

