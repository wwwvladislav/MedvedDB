#include "minunit.h"
#include "mdv_platform.h"


int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    MU_RUN_SUITE(platform);
    MU_REPORT();
    return minunit_status;
}

