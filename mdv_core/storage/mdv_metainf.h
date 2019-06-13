#pragma once
#include "mdv_types.h"
#include <mdv_uuid.h>
#include <stdbool.h>
#include <stdint.h>


#define MDV_VERSION 1


typedef struct
{
    mdv_map_field(uint32_t, uint32_t)   version;
    mdv_map_field(uint32_t, mdv_uuid)   uuid;
} mdv_metainf;


bool mdv_metainf_open       (mdv_metainf *metainf, char const *path);
void mdv_metainf_close();
void mdv_metainf_validate   (mdv_metainf *metainf);
void mdv_metainf_flush      (mdv_metainf *metainf);

