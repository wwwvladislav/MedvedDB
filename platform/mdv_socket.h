#pragma once
#include "mdv_string.h"
#include <sys/socket.h>
#include <stdbool.h>


typedef struct sockaddr_storage mdv_address_t;


bool mdv_str2addr(mdv_string_t const str, mdv_address_t *addr);
bool mdv_addr2str(mdv_address_t const *addr, mdv_string_t *str);

