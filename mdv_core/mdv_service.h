#pragma once
#include "mdv_core.h"


/// Service
typedef struct
{
    mdv_core       *core;               ///< Core component for cluster nodes management and storage accessing.
    volatile bool   is_started;         ///< Flag indicates that the service is working
} mdv_service;


bool mdv_service_create(mdv_service *svc, char const *cfg_file_path);
void mdv_service_free(mdv_service *svc);
bool mdv_service_start(mdv_service *svc);
void mdv_service_stop(mdv_service *svc);
