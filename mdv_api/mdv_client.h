#pragma once
#include <mdv_binn.h>
#include <stdbool.h>
#include <stdint.h>


struct mdv_client;


typedef struct mdv_client mdv_client;


mdv_client * mdv_client_create(char const *addr);
bool         mdv_client_connect(mdv_client *client);
void         mdv_client_disconnect(mdv_client *client);
int          mdv_client_errno(mdv_client *client);
char const * mdv_client_status_msg(mdv_client *client);
