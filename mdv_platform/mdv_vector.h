/**
 * @file mdv_vector.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief A sequence container that encapsulates dynamic size arrays.
 * @version 0.1
 * @date 2019-08-09
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_alloc.h"


/// Vector definition
#define mdv_vector(type)                        \
    struct                                      \
    {                                           \
        mdv_allocator  const *allocator;        \
        size_t               capacity;          \
        size_t               size;              \
        type                *data;              \
    }


/**
 * @brief Create new vector with given capacity
 *
 * @param vector [in]   Vector to be initialized
 * @param cpcty [in]    Vector capacity
 * @param alloctr [in]  memory allocator
 *
 * @return On success, returns non zero pointer to the array
 * @return On error, return NULL pointer
 */
#define mdv_vector_create(vector, cpcty, alloctr)                               \
    ((vector).allocator = &alloctr,                                             \
    (vector).capacity = cpcty,                                                  \
    (vector).size = 0,                                                          \
    (vector).data = (vector).allocator->alloc((cpcty) * sizeof(*(vector).data), #vector))


/**
 * @brief Frees vector created by mdv_vector_create()
 *
 * @param vector [in]   Vector to be freed
 */
#define mdv_vector_free(vector)                                             \
    if ((vector).data)                                                      \
    {                                                                       \
        (vector).capacity = 0;                                              \
        (vector).size = 0;                                                  \
        (vector).allocator->free((vector).data, #vector);                   \
    }


/**
 * @brief Appends the given element value to the end of the container
 *
 * @param vector [in]   Vector
 * @param item [in]     item to be append
 *
 * @return On success, returns non zero pointer to the new value
 * @return On error, return NULL pointer
 */
#define mdv_vector_push_back(vector, item)                                                      \
    ((vector).size < (vector).capacity                                                          \
        ? (vector).data[(vector).size++] = item, &(vector).data[(vector).size - 1]              \
        : ((vector).allocator->realloc((void**)&(vector).data, (vector).capacity * 2, #vector)  \
            ? (vector).capacity *= 2,                                                           \
              (vector).data[(vector).size++] = item,                                            \
              &(vector).data[(vector).size - 1]                                                 \
            : 0))


/**
 * @brief vector entries iterator
 *
 * @param vector [in]   vector
 * @param type [in]     vector item type
 * @param entry [out]   vector entry
 */
#define mdv_vector_foreach(vector, type, entry)         \
    for(type *entry = (vector).data,                    \
        *end = (vector).data + (vector).size;           \
        entry < end;                                    \
        ++entry)


/**
 * @brief Checks if the container has no elements
 */
#define mdv_vector_empty(vector) ((vector).size == 0)


/**
 * @brief Returns the number of elements in the container
 */
#define mdv_vector_size(vector) (vector).size