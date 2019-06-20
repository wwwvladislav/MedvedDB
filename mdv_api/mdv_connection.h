#pragma once
#include <mdv_binn.h>
#include <stdbool.h>
#include <stdint.h>


struct s_mdv_connection;


typedef struct s_mdv_connection mdv_connection;


mdv_connection * mdv_connect(char const *addr);
void             mdv_disconnect(mdv_connection *connection);
bool             mdv_send_msg(mdv_connection *connection, uint32_t id, binn *msg);