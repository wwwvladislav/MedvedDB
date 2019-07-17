#pragma once
#include <mdv_messages.h>

/// Base type for connection contexts mdv_peer and mdv_user
typedef struct mdv_conctx
{
    mdv_cli_type type;      ///< Client type
} mdv_conctx;

