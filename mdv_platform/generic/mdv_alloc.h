#pragma once
#include <mdv_def.h>


void *mdv_alloc(size_t size);
void *mdv_realloc(void *ptr, size_t size);
void  mdv_free(void *ptr);


/// Memeory allocator
typedef struct
{
    void * (*alloc)(size_t size);
    void * (*realloc)(void *ptr, size_t size);
    void (*free)(void *ptr);
} mdv_allocator;


extern mdv_allocator const mdv_default_allocator;
extern mdv_allocator const mdv_stallocator;

