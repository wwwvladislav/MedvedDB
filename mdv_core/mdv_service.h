#pragma once
#include "mdv_config.h"
#include "mdv_cluster.h"
#include "storage/mdv_metainf.h"
#include "storage/mdv_nodes.h"
#include "storage/mdv_tablespace.h"
#include <stdbool.h>
#include <mdv_uuid.h>


typedef struct
{
    mdv_cluster     cluster;
    volatile bool   is_started;
    mdv_metainf     metainf;

    struct
    {
        mdv_storage    *metainf;
        mdv_nodes       nodes;
        mdv_tablespace  tablespace;
    } storage;
} mdv_service;


bool mdv_service_create(mdv_service *svc, char const *cfg_file_path);
void mdv_service_free(mdv_service *svc);
bool mdv_service_start(mdv_service *svc);
void mdv_service_stop(mdv_service *svc);
