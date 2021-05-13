#pragma once
#include <mdv_def.h>


void *mdv_alloc(size_t size, char const *name);
void *mdv_aligned_alloc(size_t alignment, size_t size, char const *name);
void *mdv_realloc(void *ptr, size_t size, char const *name);
void  mdv_free(void *ptr, char const *name);
int   mdv_allocations();


/// Memeory allocator
typedef struct
{
    void * (*alloc)(size_t size, char const *name);
    void * (*realloc)(void *ptr, size_t size, char const *name);
    void (*free)(void *ptr, char const *name);
} mdv_allocator;


extern mdv_allocator const mdv_default_allocator;
extern mdv_allocator const mdv_stallocator;

