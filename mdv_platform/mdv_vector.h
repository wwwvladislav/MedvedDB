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
        size_t  capacity;                       \
        size_t  size;                           \
        type   *data;                           \
    }


/**
 * @brief Create new vector with given capacity
 *
 * @param vector [in]   Vector to be initialized
 * @param cpcty [in]    Vector capacity
 *
 * @return On success, returns non zero pointer to the array
 * @return On error, return NULL pointer
 */
#define mdv_vector_create(vector, cpcty)                                    \
    ((vector).capacity = cpcty,                                             \
    (vector).size = 0,                                                      \
    (vector).data = mdv_alloc((cpcty) * sizeof(*(vector).data), #vector))


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
        mdv_free((vector).data, #vector);                                   \
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
#define mdv_vector_push_back(vector, item)                                              \
    ((vector).size < (vector).capacity                                                  \
        ? (vector).data[(vector).size++] = item, &(vector).data[(vector).size - 1]      \
        : (mdv_realloc2((void**)&(vector).data, (vector).capacity * 2, #vector)         \
            ? (vector).capacity *= 2,                                                   \
              (vector).data[(vector).size++] = item,                                    \
              &(vector).data[(vector).size - 1]                                         \
            : 0))
