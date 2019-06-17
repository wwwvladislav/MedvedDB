#pragma once
#include <stdint.h>


uint32_t mdv_hash_murmur2a(uint8_t const *data, uint32_t const len,  uint32_t const seed);

