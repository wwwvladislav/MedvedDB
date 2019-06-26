#pragma once
#include <math.h>
#include <stdint.h>
#include <stdbool.h>


typedef struct mdv_bloom mdv_bloom;


mdv_bloom * mdv_bloom_create(uint32_t entries, double err);
void        mdv_bloom_free(mdv_bloom *bloom);
bool        mdv_bloom_insert(mdv_bloom *bloom, void const *data, uint32_t const len);
bool        mdv_bloom_contains(mdv_bloom *bloom, void const *data, uint32_t const len);
uint32_t    mdv_bloom_capacity(mdv_bloom const *bloom);
uint32_t    mdv_bloom_size(mdv_bloom const *bloom);

