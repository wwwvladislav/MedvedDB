#pragma once


enum
{
    MDV_OK = 0,
    MDV_FAILED,
    MDV_INVALID_ARG,
    MDV_EAGAIN,
    MDV_CLOSED
};


typedef int mdv_errno;

