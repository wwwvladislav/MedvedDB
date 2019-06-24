#include "mdv_status.h"


char const * mdv_status_message(int status)
{
    switch(status)
    {
        case MDV_STATUS_OK:                         return "OK";
        case MDV_STATUS_INVALID_PROTOCOL_VERSION:   return "Invalid protocol version";
        case MDV_STATUS_FAILED:                     return "Operation failed";
        case MDV_STATUS_NOT_IMPL:                   return "Functionality wasn't ipmlemented";
    }

    return "Unknown error";
}
