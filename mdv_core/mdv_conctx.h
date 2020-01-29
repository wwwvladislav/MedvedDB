#pragma once
#include <mdv_proto.h>
#include <mdv_def.h>


/// Connection context
typedef struct mdv_conctx mdv_conctx;


/// Interface for connection contexts
typedef struct
{
    mdv_conctx * (*retain)(mdv_conctx *);       ///< Function for connection context retain
    uint32_t (*release)(mdv_conctx *);          ///< Function for connection context release
} mdv_iconctx;


/// Connection context
struct mdv_conctx
{
    mdv_iconctx    *vptr;                       ///< Virtual method table
    uint8_t         type;                       ///< Connection context type
};

