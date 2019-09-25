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


static mdv_vector empty_vector = {};
mdv_vector *mdv_empty_vector = &empty_vector;


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


mdv_vector * mdv_vector_retain(mdv_vector *vector)
{
    if (vector == mdv_empty_vector)
        return vector;
    atomic_fetch_add_explicit(&vector->rc, 1, memory_order_acquire);
    return vector;
}


void mdv_vector_release(mdv_vector *vector)
{
    if (vector
        && vector != mdv_empty_vector
        && atomic_fetch_sub_explicit(&vector->rc, 1, memory_order_release) == 1)
    {
        vector->allocator->free(vector->data, "vector.data");
        vector->allocator->free(vector, "vector");
    }
}


void * mdv_vector_data(mdv_vector *vector)
{
    return vector->data;
}


size_t mdv_vector_size(mdv_vector *vector)
{
    return vector->size;
}


size_t mdv_vector_capacity(mdv_vector *vector)
{
    return vector->capacity;
}


void * mdv_vector_push_back(mdv_vector *vector, void const *item)
{
    if (vector->size >= vector->capacity)
    {
        if (!vector->allocator->realloc((void**)&vector->data, vector->capacity * 2, "vector.data"))
            return 0;
        vector->capacity *= 2;
    }

    void *ptr = vector->data + vector->size * vector->item_size;

    memcpy(ptr, item, vector->item_size);

    vector->size++;

    return ptr;
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
