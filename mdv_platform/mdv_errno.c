#include "mdv_errno.h"
#include <string.h>
#include <errno.h>


mdv_errno mdv_error()
{
    switch(errno)
    {
        case EAGAIN:        return MDV_EAGAIN;
        case EEXIST:        return MDV_EEXIST;
        case EINPROGRESS:   return MDV_INPROGRESS;
        case ETIMEDOUT:     return MDV_ETIMEDOUT;
    }
    return errno;
}


char const * mdv_strerror(mdv_errno err)
{
    switch(err)
    {
        case MDV_OK:            return "Operation successfully completed";
        case MDV_FAILED:        return "Unknown error";
        case MDV_INVALID_ARG:   return "Function argument is invalid";
        case MDV_EAGAIN:        return "Resource temporarily unavailable";
        case MDV_CLOSED:        return "File descriptor closed";
        case MDV_EEXIST:        return "File exists";
        case MDV_INPROGRESS:    return "Operation now in progress";
        case MDV_ETIMEDOUT:     return "Timed out";
        case MDV_BUSY:          return "Resource is busy";
        case MDV_NO_IMPL:       return "No implementaion";
    }

    static _Thread_local char buf[1024];

    strerror_r(err, buf, sizeof buf);
    buf[sizeof buf - 1] = 0;

    return buf;
}

