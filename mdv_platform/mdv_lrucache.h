/**
 * @file mdv_lrucache.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief LRU cache
 * @version 0.1
 * @date 2020-04-25
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_def.h"


/// LRU cache key type
typedef uint32_t mdv_lrukey;


/// LRU cache item
typedef struct mdv_lruitem
{
    mdv_lrukey key;     ///< key
    void      *data;    ///< data associated with key
} mdv_lruitem;


/// LRU cache
typedef struct mdv_lrucache mdv_lrucache;


/**
 * @brief LRU cache creation with given capacity
 *
 * @param capacity [in] capacity
 *
 * @return On success returns non zero pointer to new created LRU cache,
 *         otherwise returns NULL pointer.
 */
mdv_lrucache * mdv_lrucache_create(size_t capacity);


/**
 * @brief Retains LRU cache.
 * @details Reference counter is increased by one.
 */
mdv_lrucache * mdv_lrucache_retain(mdv_lrucache *cache);


/**
 * @brief Releases LRU cache.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the LRU cache is deleted.
 */
uint32_t mdv_lrucache_release(mdv_lrucache *cache);


/**
 * @brief Put data into the LRU cache
 *
 * @param cache [in]    LRU cache
 * @param new_item [in] new item
 * @param old [out]     outdated value
 *
 * @return insertion result
 */
bool mdv_lrucache_put(mdv_lrucache *cache, mdv_lruitem new_item, mdv_lruitem *old);


/**
 * @brief Search data associated with key in LRU cache
 *
 * @param cache [in]    LRU cache
 * @param key [in]      key
 *
 * @return On success returns pointer to the found data, otherwise returns NULL.
 */
void * mdv_lrucache_get(mdv_lrucache *cache, mdv_lrukey key);
