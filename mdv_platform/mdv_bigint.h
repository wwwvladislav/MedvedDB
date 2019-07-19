#pragma once
#include "mdv_def.h"


#define mdv_bigint(S)                   \
    struct                              \
    {                                   \
        uint8_t     size;               \
        uint8_t     n[S];               \
    }


typedef mdv_bigint(1) mdv_bigint_base;
