#include "mdv_time.h"
#include "mdv_log.h"
#include <time.h>


size_t mdv_gettime()
{
    struct timespec tp = {};

    if (clock_gettime(CLOCK_REALTIME, &tp) != 0)
    {
        mdv_errno err = mdv_error();
        char err_msg[128];
        MDV_LOGE("gettime failed with error: '%s' (%d)",
                mdv_strerror(err, err_msg, sizeof err_msg), err);
    }

    return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
}

