#pragma once
#include <mdv_map.h>
#include <mdv_lmdb.h>
#include <mdv_uuid.h>
#include <mdv_version.h>
#include <stdbool.h>
#include <stdint.h>


typedef struct
{
    mdv_map_field(uint32_t, uint32_t)   version;
    mdv_map_field(uint32_t, mdv_uuid)   uuid;
} mdv_metainf;


mdv_lmdb * mdv_metainf_storage_open(char const *path);
bool       mdv_metainf_load        (mdv_metainf *metainf, mdv_lmdb *storage);
void       mdv_metainf_validate    (mdv_metainf *metainf);
void       mdv_metainf_flush       (mdv_metainf *metainf, mdv_lmdb *storage);

