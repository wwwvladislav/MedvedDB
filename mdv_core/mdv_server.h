#pragma once
#include "storage/mdv_tablespace.h"
#include "storage/mdv_nodes.h"


typedef struct mdv_server mdv_server;


mdv_server * mdv_server_create(mdv_tablespace *tablespace, mdv_nodes *nodes, mdv_uuid const *uuid);
bool         mdv_server_start(mdv_server *srvr);
void         mdv_server_free(mdv_server *srvr);

