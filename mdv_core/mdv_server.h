#pragma once
#include <mdv_handler.h>


struct mdv_server;
typedef struct mdv_server mdv_server;


mdv_server * mdv_server_create();
bool         mdv_server_start(mdv_server *srvr);
void         mdv_server_free(mdv_server *srvr);
bool         mdv_server_handler_reg(mdv_server *srvr, uint32_t msg_id, mdv_message_handler handler);

