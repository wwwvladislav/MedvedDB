#pragma once
#include <stddef.h>
#include <stdint.h>


int   mdv_alloc_initialize();
void  mdv_alloc_finalize();
void  mdv_alloc_thread_initialize();
void  mdv_alloc_thread_finalize();
void *mdv_alloc(size_t size);
void *mdv_aligned_alloc(size_t alignment, size_t size);
void  mdv_free(void *ptr);

