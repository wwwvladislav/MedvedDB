#pragma once
#include "mdv_map.h"
#include "mdv_storage.h"
#include <mdv_uuid.h>
#include <mdv_types.h>
#include <stdbool.h>
#include <stdint.h>


#define MDV_VERSION 1


typedef struct
{
    mdv_map_field(uint32_t, uint32_t)   version;
    mdv_map_field(uint32_t, mdv_uuid)   uuid;
} mdv_metainf;


mdv_storage * mdv_metainf_storage_open(char const *path);
bool          mdv_metainf_load        (mdv_metainf *metainf, mdv_storage *storage);
void          mdv_metainf_validate    (mdv_metainf *metainf);
void          mdv_metainf_flush       (mdv_metainf *metainf, mdv_storage *storage);

