#pragma once
#include <stddef.h>
#include <stdint.h>


void *mdv_alloc(size_t size);
void *mdv_aligned_alloc(size_t alignment, size_t size);
void  mdv_free(void *ptr);

