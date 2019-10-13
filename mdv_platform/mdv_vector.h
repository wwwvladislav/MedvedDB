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
typedef struct mdv_vector mdv_vector;


extern mdv_vector mdv_empty_vector;


/**
 * @brief Create new vector with given capacity
 *
 * @param capacity [in]     capacity
 * @param item_size [in]    item size
 * @param allocator [in]    memory allocator
 *
 * @return On success, returns non zero pointer to the vector
 * @return On error, return NULL pointer
 */
mdv_vector * mdv_vector_create(size_t capacity, size_t item_size, mdv_allocator const *allocator);


/**
 * @brief Creates copy of vector with given capacity
 *
 * @param vector [in]   Vector
 * @param capacity [in] New vector capacity
 *
 * @return On success, returns non zero pointer to the vector
 * @return On error, return NULL pointer
 */
mdv_vector * mdv_vector_clone(mdv_vector const *vector, size_t capacity);


/**
 * @brief Retains vector.
 * @details Reference counter is increased by one.
 */
mdv_vector * mdv_vector_retain(mdv_vector *vector);


/**
 * @brief Releases vector.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the vecror's destructor is called.
 */
void mdv_vector_release(mdv_vector *vector);


/**
 * @brief Returns pointer to the underlying array serving as element storage.
 */
void * mdv_vector_data(mdv_vector *vector);


/**
 * @brief Return vector size
 */
size_t mdv_vector_size(mdv_vector const *vector);


/**
 * @brief Resizes the vector so that it contains n elements.
 */
bool mdv_vector_resize(mdv_vector *vector, size_t n);


/**
 * @brief Return vector capacity
 */
size_t mdv_vector_capacity(mdv_vector const *vector);


/**
 * @brief Appends the given element to the end of the container
 *
 * @param vector [in]   Vector
 * @param item [in]     item to be append
 *
 * @return On success, returns non zero pointer to the new value
 * @return On error, return NULL pointer
 */
void * mdv_vector_push_back(mdv_vector *vector, void const *item);


/**
 * @brief Appends the given elements to the end of the container
 *
 * @param vector [in]   Vector
 * @param items [in]    items to be append
 * @param count [in]    items count
 *
 * @return On success, returns non zero pointer to the first inserted value
 * @return On error, return NULL pointer
 */
void * mdv_vector_append(mdv_vector *vector, void const *items, size_t count);


/**
 * @brief vector entries iterator
 *
 * @param vector [in]   vector
 * @param type [in]     vector item type
 * @param entry [out]   vector entry
 */
#define mdv_vector_foreach(vector, type, entry)         \
    for(type *entry = mdv_vector_data(vector),          \
        *end = entry + mdv_vector_size(vector);         \
        entry < end;                                    \
        ++entry)


/**
 * @brief Checks if the container has no elements
 */
#define mdv_vector_empty(vector) (mdv_vector_size(vector) == 0)


/**
 * @brief Erases all elements from the container. After this call, mdv_vector_size() returns zero.
 */
void mdv_vector_clear(mdv_vector *vector);


/**
 * @brief Returns a reference to the element at specified location pos, with bounds checking.
 */
void * mdv_vector_at(mdv_vector *vector, size_t pos);


/**
 * @brief Searches specified item in vector
 *
 * @param vector [in]   vector
 * @param item [in]     item for search
 * @param equ [in]      predicate to compare two items
 *
 * @return pointer to the element
 */
void * mdv_vector_find(mdv_vector *vector,
                       void const *item,
                       bool (*equ)(void const *, void const *));


/**
 * @brief Erases the specified elements from the container.
 *
 * @param vector [in]   vector
 * @param item [in]     item for erase
 */
void mdv_vector_erase(mdv_vector *vector, void const *item);

