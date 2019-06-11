#pragma once
#include "mdv_config.h"
#include "mdv_listener.h"
#include "storage/mdv_metainf.h"
#include <stdbool.h>
#include <mdv_uuid.h>


typedef struct
{
    mdv_config      config;
    mdv_listener    listener;

    struct
    {
        mdv_metainf metainf;
    } db;
} mdv_service;


bool mdv_service_init(mdv_service *svc, char const *cfg_file_path);
void mdv_service_free(mdv_service *svc);
bool mdv_service_start(mdv_service *svc);
