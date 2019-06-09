#pragma once
#include <nng/nng.h>
#include <stdbool.h>
#include "mdv_config.h"


typedef struct
{
    nng_socket  sock;
} mdv_listener;


bool mdv_listener_init(mdv_listener *lstnr, mdv_config const *cfg);
void mdv_listener_free(mdv_listener *lstnr);
