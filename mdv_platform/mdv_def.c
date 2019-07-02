#include "mdv_def.h"
#include <unistd.h>


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

