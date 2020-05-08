#include "minunit.h"
#include "mdv_platform.h"
#include "mdv_types.h"
#include "mdv_core.h"
#include "mdv_crypto.h"
#include <mdv_alloc.h>
#include <mdv_log.h>


int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    mdv_logf_set_level(ZF_LOG_INFO);

    mdv_alloc_initialize();

    MU_RUN_SUITE(platform);
    MU_RUN_SUITE(types);
    MU_RUN_SUITE(core);
    MU_RUN_SUITE(crypto);
    MU_REPORT();

    mdv_alloc_finalize();

    printf("Allocations: %d\n", mdv_allocations());

    return minunit_status;
}

