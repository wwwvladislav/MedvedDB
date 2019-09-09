#include "mdv_def.h"
#include <unistd.h>


size_t mdv_descriptor_hash(mdv_descriptor const *fd)
{
    return (size_t)*fd;
}


int mdv_descriptor_cmp(mdv_descriptor const *fd1, mdv_descriptor const *fd2)
{
    return (int)(*fd1 - *fd2);
}


mdv_errno mdv_write(mdv_descriptor fd, void const *data, size_t *len)
{
    if (!*len)
        return MDV_OK;

    int res = write(*(int*)&fd, data, *len);

    if (res == -1)
        return mdv_error();

    *len = (size_t)res;

    return MDV_OK;
}


mdv_errno mdv_write_all(mdv_descriptor fd, void const *data, size_t len)
{
    size_t total = 0;

    while(total < len)
    {
        size_t wlen = len - total;

        mdv_errno err = mdv_write(fd, data, &wlen);

        switch (err)
        {
            case MDV_EAGAIN:
                continue;

            case MDV_OK:
            {
                data = (char const *)data + wlen;
                total += wlen;
                continue;
            }

            default:
                return err;
        }
    }

    return MDV_OK;
}


mdv_errno mdv_read(mdv_descriptor fd, void *data, size_t *len)
{
    if (!*len)
        return MDV_OK;

    int res = read(*(int*)&fd, data, *len);

    switch(res)
    {
        case -1:    return mdv_error();
        case 0:     return MDV_CLOSED;
        default:    break;
    }

    *len = (size_t)res;

    return MDV_OK;
}

