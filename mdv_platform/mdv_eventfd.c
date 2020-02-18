#include "mdv_eventfd.h"
#include "mdv_log.h"
#include <unistd.h>

#ifdef MDV_PLATFORM_LINUX
    #include <sys/eventfd.h>
#endif


mdv_descriptor mdv_eventfd(bool semaphore)
{
    int flags = EFD_CLOEXEC | EFD_NONBLOCK;

    if (semaphore)
        flags |= EFD_SEMAPHORE;

    int ret = eventfd(0, flags);

    if (ret == -1)
    {
        int err = mdv_error();
        char err_msg[128];
        MDV_LOGE("eventfd creation failed with error: '%s' (%d)", mdv_strerror(err, err_msg, sizeof err_msg), err);
        return MDV_INVALID_DESCRIPTOR;
    }

    MDV_LOGD("eventfd %d opened", ret);

    mdv_descriptor fd = 0;

    *(int*)&fd = ret;

    return fd;
}


void mdv_eventfd_close(mdv_descriptor fd)
{
    if (fd != MDV_INVALID_DESCRIPTOR)
    {
        int evt_fd = *(int*)&fd;
        close(evt_fd);
        MDV_LOGD("eventfd %d closed", evt_fd);
    }
}

