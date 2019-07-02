#include "mdv_pipe.h"
#include "mdv_log.h"
#include "mdv_errno.h"
#include <unistd.h>


mdv_errno mdv_pipe_open(mdv_pipe *p)
{
    int pipefd[2];

    if (pipe(pipefd) == -1)
    {
        MDV_LOGE("Unable to create new pipe");
        return mdv_error();
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

