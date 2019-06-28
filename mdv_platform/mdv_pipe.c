#include "mdv_pipe.h"
#include "mdv_log.h"
#include <unistd.h>
#include <errno.h>


mdv_errno mdv_pipe_open(mdv_pipe *p)
{
    int pipefd[2];

    if (pipe(pipefd) == -1)
    {
        MDV_LOGE("Unable to create new pipe");
        return errno;
    }

    *(size_t*)&p->rd = pipefd[0];
    *(size_t*)&p->wd = pipefd[1];

    return MDV_OK;
}


void mdv_pipe_close(mdv_pipe *p)
{
    close((int)(size_t)p->rd);
    close((int)(size_t)p->wd);
    p->rd = MDV_INVALID_DESCRIPTOR;
    p->wd = MDV_INVALID_DESCRIPTOR;
}


mdv_errno pipe_write(mdv_pipe *p, void const *buf, size_t len)
{
    int wd = (int)(size_t)p->wd;

    ssize_t ret = write(wd, buf, len);

    return ret == -1 ? errno
            : ret == len ? MDV_OK
            : -1;
}


mdv_errno pipe_read(mdv_pipe *p, void *buf, size_t len)
{
    int rd = (int)(size_t)p->rd;

    ssize_t ret = read(rd, buf, len);

    return ret == -1 ? errno
            : ret == len ? MDV_OK
            : -1;
}

