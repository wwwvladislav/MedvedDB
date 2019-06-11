#pragma once
#include <mdv_uuid.h>
#include <stdbool.h>
#include <stdint.h>


#define MDV_VERSION 1


typedef struct
{
    uint32_t version;
    mdv_uuid uuid;
} mdv_metainf;


bool mdv_metainf_open(mdv_metainf *metainf, char const *path);
void mdv_metainf_close();

