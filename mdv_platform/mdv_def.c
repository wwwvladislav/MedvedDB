#include "mdv_def.h"
#include "mdv_log.h"
#include <unistd.h>


size_t mdv_descriptor_hash(mdv_descriptor const *fd)
{
    return (size_t)*fd;
}


int mdv_descriptor_cmp(mdv_descriptor const *fd1, mdv_descriptor const *fd2)
{
    return (int)(*fd1 - *fd2);
}


void mdv_descriptor_close(mdv_descriptor fd)
{
    if (fd != MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGD("descriptor %d closed", *(int*)&fd);
        close(*(int*)&fd);
    }
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


mdv_errno mdv_read_all(mdv_descriptor fd, void *data, size_t len)
{
    if (!len)
        return MDV_OK;

    size_t total = 0;

    while(total < len)
    {
        size_t rlen = len - total;

        mdv_errno err = mdv_read(fd, data, &rlen);

        switch (err)
        {
            case MDV_EAGAIN:
                continue;

            case MDV_OK:
            {
                data = (char *)data + rlen;
                total += rlen;
                continue;
            }

            default:
                return err;
        }
    }

    return MDV_OK;
}


mdv_errno mdv_skip(mdv_descriptor fd, size_t len)
{
    char buf[64];

    size_t skipped_len = 0;

    while(skipped_len < len)
    {
        size_t skip_size = sizeof buf > len - skipped_len
                            ? len - skipped_len
                            : sizeof buf;

        int res = read(*(int*)&fd, buf, skip_size);

        switch(res)
        {
            case -1:    return mdv_error();
            case 0:     return MDV_CLOSED;
            default:    break;
        }

        skipped_len += res;
    }

    return MDV_OK;
}
