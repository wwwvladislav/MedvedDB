#pragma once
#include "storage/mdv_metainf.h"
#include "storage/mdv_tablespace.h"
#include <stdbool.h>
#include <mdv_cluster.h>


/// Service
typedef struct
{
    mdv_cluster     cluster;            ///< Cluster manager
    volatile bool   is_started;         ///< Flag indicates that the service is working
    mdv_metainf     metainf;            ///< Metainformation (DB version, node UUID etc.)

    struct
    {
        mdv_storage    *metainf;        ///< Metainformation storage
        mdv_tablespace  tablespace;     ///< Tables strorage
    } storage;                          ///< Storages
} mdv_service;


bool mdv_service_create(mdv_service *svc, char const *cfg_file_path);
void mdv_service_free(mdv_service *svc);
bool mdv_service_start(mdv_service *svc);
void mdv_service_stop(mdv_service *svc);
