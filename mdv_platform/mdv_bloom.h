#pragma once
#include <math.h>
#include <stddef.h>
#include <stdint.h>


struct mdv_bloom_s;


typedef struct mdv_bloom_s mdv_bloom;


mdv_bloom * mdv_bloom_create(uint32_t entries, double err);
void        mdv_bloom_free(mdv_bloom *bloom);

