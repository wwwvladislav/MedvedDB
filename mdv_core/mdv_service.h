#pragma once
#include "mdv_config.h"
#include "mdv_listener.h"
#include <stdbool.h>


typedef struct
{
    mdv_config      config;
    mdv_listener    listener;
} mdv_service;


bool mdv_service_init(mdv_service *svc, char const *cfg_file_path);
void mdv_service_free(mdv_service *svc);
bool mdv_service_start(mdv_service *svc);
