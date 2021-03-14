#include "mdv_file.h"
#include "mdv_log.h"

#ifdef MDV_PLATFORM_LINUX
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
#endif


static int mdv_oflags2sys(int flags)
{
    int f = 0;

    if ((flags & MDV_OREAD) && (flags & MDV_OWRITE))
        f |= O_RDWR;
    else if (flags & MDV_OREAD)
        f |= O_RDONLY;
    else if (flags & MDV_OWRITE)
        f |= O_WRONLY;

    if (flags & MDV_OCREAT)      f |= O_CREAT;
    if (flags & MDV_ODIRECT)     f |= O_DIRECT;
    if (flags & MDV_ODSYNC)      f |= O_DSYNC;
    if (flags & MDV_OLARGEFILE)  f |= O_LARGEFILE;
    if (flags & MDV_ONOATIME)    f |= O_NOATIME;
    if (flags & MDV_OTMPFILE)    f |= O_TMPFILE;

    return f;
}


mdv_descriptor mdv_open(const char *pathname, int flags)
{
    int ret = open(pathname, mdv_oflags2sys(flags));

    if (ret == -1)
    {
        int err = mdv_error();
        char err_msg[128];
        MDV_LOGE("File opening failed with error: '%s' (%d)", mdv_strerror(err, err_msg, sizeof err_msg), err);
        return MDV_INVALID_DESCRIPTOR;
    }

    MDV_LOGD("file %d opened", ret);

    mdv_descriptor fd = 0;
    *(int*)&fd = ret;

    return fd;
}


mdv_errno mdv_file_size_by_path(const char *path, size_t *size)
{
    if (!path || !size)
        return MDV_INVALID_ARG;

    struct stat st;

    if (stat(path, &st) != 0)
        return MDV_FAILED;

    *size = st.st_size;

    return MDV_OK;
}


mdv_errno mdv_file_size_by_fd(mdv_descriptor fd, size_t *size)
{
    if (fd == MDV_INVALID_DESCRIPTOR || !size)
        return MDV_INVALID_ARG;

    struct stat st;

    if (fstat(*(int*)&fd, &st) != 0)
        return MDV_FAILED;

    *size = st.st_size;

    return MDV_OK;
}

