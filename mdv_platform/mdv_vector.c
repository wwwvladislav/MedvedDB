#include "mdv_vector.h"
#include "mdv_log.h"
#include <stdatomic.h>
#include <string.h>


struct mdv_vector
{
    atomic_uint_fast32_t  rc;
    mdv_allocator  const *allocator;
    size_t                capacity;
    size_t                size;
    size_t                item_size;
    uint8_t              *data;
};


mdv_vector mdv_empty_vector = {};


mdv_vector * mdv_vector_create(size_t capacity, size_t item_size, mdv_allocator const *allocator)
{
    mdv_vector *vector = allocator->alloc(sizeof(mdv_vector), "vector");

    if (!vector)
    {
        MDV_LOGE("No memory for vector");
        return 0;
    }

    atomic_init(&vector->rc, 1);

    vector->allocator = allocator;
    vector->capacity = capacity;
    vector->size = 0;
    vector->item_size = item_size;
    vector->data = allocator->alloc(capacity * item_size, "vector.data");

    if (!vector->data)
    {
        MDV_LOGE("No memory for vector");
        allocator->free(vector, "vector");
        return 0;
    }

    return vector;
}


mdv_vector * mdv_vector_clone(mdv_vector const *vector, size_t capacity)
{
    if (capacity < vector->size)
        capacity = vector->size;

    mdv_vector *new_vector = mdv_vector_create(capacity, vector->item_size, vector->allocator);

    if (!new_vector)
        return 0;

    memcpy(new_vector->data,
           vector->data,
           vector->item_size * vector->size);

    new_vector->size = vector->size;

    return new_vector;
}


mdv_vector * mdv_vector_retain(mdv_vector *vector)
{
    if (vector == &mdv_empty_vector)
        return vector;
    atomic_fetch_add_explicit(&vector->rc, 1, memory_order_acquire);
    return vector;
}


uint32_t mdv_vector_release(mdv_vector *vector)
{
    if (!vector
        || vector == &mdv_empty_vector)
        return 0;

    uint32_t refs = atomic_fetch_sub_explicit(&vector->rc, 1, memory_order_release) - 1;

    if (!refs)
    {
        vector->allocator->free(vector->data, "vector.data");
        vector->allocator->free(vector, "vector");
    }

    return refs;
}


uint32_t mdv_vector_refs(mdv_vector *vector)
{
    return atomic_load_explicit(&vector->rc, memory_order_relaxed);
}


void * mdv_vector_data(mdv_vector *vector)
{
    return vector->data;
}


size_t mdv_vector_size(mdv_vector const *vector)
{
    return vector->size;
}


size_t mdv_vector_capacity(mdv_vector const *vector)
{
    return vector->capacity;
}


void * mdv_vector_push_back(mdv_vector *vector, void const *item)
{
    if (vector->size >= vector->capacity)
    {
        if (!vector->allocator->realloc((void**)&vector->data, vector->item_size * vector->capacity * 2, "vector.data"))
            return 0;
        vector->capacity *= 2;
    }

    void *ptr = vector->data + vector->size * vector->item_size;

    memcpy(ptr, item, vector->item_size);

    vector->size++;

    return ptr;
}


void * mdv_vector_append(mdv_vector *vector, void const *items, size_t count)
{
    if (vector->size + count > vector->capacity)
    {
        size_t new_capacity = vector->capacity;

        while(vector->size + count > new_capacity)
            new_capacity *= 2;

        if (!vector->allocator->realloc((void**)&vector->data, vector->item_size * new_capacity, "vector.data"))
            return 0;

        vector->capacity = new_capacity;
    }

    void *ptr = vector->data + vector->size * vector->item_size;

    memcpy(ptr, items, vector->item_size * count);

    vector->size += count;

    return ptr;
}


bool mdv_vector_resize(mdv_vector *vector, size_t n)
{
    if (n <= vector->size)
    {
        vector->size = n;

        return true;
    }

    if (n <= vector->capacity)
    {
        void *end = vector->data + vector->size * vector->item_size;

        memset(end, 0, (n - vector->size) * vector->item_size);

        vector->size = n;

        return true;
    }

    size_t new_capacity = vector->capacity;

    while(n > new_capacity)
        new_capacity *= 2;

    if (!vector->allocator->realloc((void**)&vector->data, vector->item_size * new_capacity, "vector.data"))
        return false;

    vector->capacity = new_capacity;

    void *end = vector->data + vector->size * vector->item_size;

    memset(end, 0, (n - vector->size) * vector->item_size);

    vector->size = n;

    return true;
}


bool mdv_vector_empty(mdv_vector const *vector)
{
    return !mdv_vector_size(vector);
}


void mdv_vector_clear(mdv_vector *vector)
{
    vector->size = 0;
}


void * mdv_vector_at(mdv_vector *vector, size_t pos)
{
    if (pos >= vector->size)
        return 0;
    return vector->data + pos * vector->item_size;
}


void * mdv_vector_find(mdv_vector *vector,
                       void const *item,
                       bool (*equ)(void const *, void const *))
{
    for(size_t i = 0; i < vector->size; ++i)
    {
        void *ptr = vector->data + vector->item_size * i;
        if(equ(ptr, item))
            return ptr;
    }
    return 0;
}


void mdv_vector_erase(mdv_vector *vector, void const *item)
{
    uint8_t const *end = vector->data + vector->item_size * vector->size;

    if ((uint8_t const *)item >= vector->data
        && (uint8_t const *)item < end)
    {
        size_t const idx = ((uint8_t const *)item - vector->data) / vector->item_size;

        if (vector->data + idx * vector->item_size == (uint8_t const *)item)
        {
            for(uint8_t *dst = vector->data + idx * vector->item_size,
                        *src = dst + vector->item_size;
                        src < end;
                        dst = src,
                        src += vector->item_size)
            {
                memcpy(dst, src, vector->item_size);
            }

            vector->size--;
        }
        else
            MDV_LOGE("Invalid pointer is erased from vector");
    }
    else
        MDV_LOGE("Vector bounds violation");
}
