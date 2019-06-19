#pragma once
#include <nng/nng.h>
#include <stdbool.h>


typedef struct
{
    nng_socket  sock;
    nng_aio    *aio;
} mdv_server_work;


typedef struct
{
    nng_socket       sock;

    struct
    {
        uint32_t         count;
        mdv_server_work *list;
    } works;
} mdv_server;


bool mdv_server_init(mdv_server *srvr);
void mdv_server_free(mdv_server *srvr);
