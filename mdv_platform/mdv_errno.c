#include "mdv_errno.h"
#include <string.h>
#include <errno.h>


mdv_errno mdv_error()
{
    switch(errno)
    {
        case ENOENT:        return MDV_ENOENT;
        case EAGAIN:        return MDV_EAGAIN;
        case EEXIST:        return MDV_EEXIST;
        case EINPROGRESS:   return MDV_INPROGRESS;
        case ETIMEDOUT:     return MDV_ETIMEDOUT;
    }
    return errno;
}


char const * mdv_strerror(mdv_errno err, char *buf, size_t size)
{
    if (!buf || !size)
        return "Unknown error";

    switch(err)
    {
        case MDV_OK:                return "Operation successfully completed";
        case MDV_FALSE:             return "Operation successfully completed with FALSE value";
        case MDV_FAILED:            return "Unknown error";
        case MDV_INVALID_ARG:       return "Function argument is invalid";
        case MDV_INVALID_TYPE:      return "Invalid type";
        case MDV_EAGAIN:            return "Resource temporarily unavailable";
        case MDV_CLOSED:            return "File descriptor closed";
        case MDV_EEXIST:            return "File exists";
        case MDV_INPROGRESS:        return "Operation now in progress";
        case MDV_ETIMEDOUT:         return "Timed out";
        case MDV_BUSY:              return "Resource is busy";
        case MDV_NO_IMPL:           return "No implementaion";
        case MDV_STACK_OVERFLOW:    return "Stack overflow";
        case MDV_ENOENT:            return "No such file or directory";
    }

    strerror_r(err, buf, size);
    buf[size - 1] = 0;

    return buf;
}

