#include "mdv_eventfd.h"
#include "mdv_log.h"
#include <unistd.h>

#ifdef MDV_PLATFORM_LINUX
    #include <sys/eventfd.h>
#endif


mdv_descriptor mdv_eventfd()
{
    int ret = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

    if (ret == -1)
    {
        int err = mdv_error();
        MDV_LOGE("eventfd creation was failed with error: '%s' (%d)", mdv_strerror(err), err);
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

