#include "mdv_timerfd.h"
#include <sys/timerfd.h>
#include "mdv_log.h"
#include <unistd.h>

#ifdef MDV_PLATFORM_LINUX
    #include <sys/timerfd.h>
#endif


mdv_descriptor mdv_timerfd()
{
    int ret = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    if (ret == -1)
    {
        int err = mdv_error();
        MDV_LOGE("timerfd creation failed with error: '%s' (%d)", mdv_strerror(err), err);
        return MDV_INVALID_DESCRIPTOR;
    }

    MDV_LOGD("timerfd %d opened", ret);

    mdv_descriptor fd = 0;

    *(int*)&fd = ret;

    return fd;
}


void mdv_timerfd_close(mdv_descriptor fd)
{
    if (fd != MDV_INVALID_DESCRIPTOR)
    {
        int tfd = *(int*)&fd;
        close(tfd);
        MDV_LOGD("timerfd %d closed", tfd);
    }
}


mdv_errno mdv_timerfd_settime(mdv_descriptor fd, size_t start, size_t interval)
{
    struct itimerspec timerspec =
    {
        .it_interval =
        {
            .tv_sec = interval / 1000,
            .tv_nsec = (interval % 1000) * 1000000
        },

        .it_value =
        {
            .tv_sec = start / 1000,
            .tv_nsec = (start % 1000) * 1000000
        }
    };

    int tfd = *(int*)&fd;

    if (timerfd_settime(tfd, TFD_TIMER_ABSTIME, &timerspec, 0) != 0)
    {
        mdv_errno err = mdv_error();
        MDV_LOGE("timerfd_settime failed with error: '%s' (%d)", mdv_strerror(err), err);
        return err;
    }

    return MDV_OK;
}


size_t mdv_gettime()
{
    struct timespec tp = {};

    if (clock_gettime(CLOCK_REALTIME, &tp) != 0)
    {
        mdv_errno err = mdv_error();
        MDV_LOGE("gettime failed with error: '%s' (%d)", mdv_strerror(err), err);
    }

    return tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
}

