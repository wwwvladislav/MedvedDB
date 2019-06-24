#pragma once


enum
{
    MDV_STATUS_OK                       = 1,
    MDV_STATUS_INVALID_PROTOCOL_VERSION = 2,
    MDV_STATUS_FAILED           = 3,
    MDV_STATUS_NOT_IMPL         = 4,
};


char const * mdv_status_message(int status);
