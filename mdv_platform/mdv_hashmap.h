/**
 * @brief Hash map.
 */
#pragma once
#include "mdv_def.h"
#include "mdv_list.h"


/// Hash map entry
typedef mdv_list mdv_hashmap_bucket;


/// Hash function type
typedef size_t (*mdv_hash_fn)(void const *);


/// Keys comparison function type
typedef int (*mdv_key_cmp_fn)(void const *, void const *);


/// Hash map
typedef struct mdv_hashmap mdv_hashmap;


/// @cond Doxygen_Suppress


mdv_hashmap * _mdv_hashmap_create(size_t         capacity,
                                  uint32_t       key_offset,
                                  uint32_t       key_size,
                                  mdv_hash_fn    hash_fn,
                                  mdv_key_cmp_fn key_cmp_fn);


/// @endcond


/**
 * @brief Initialize hash map.
 *
 * @details Allocates space for hash map and initializes fields.
 *
 * @param hm [in]         hash map
 * @param type [in]       hash map items type
 * @param key_field [in]  key field name in type
 * @param capacity [in]   hash map capacity
 * @param hash_fn [in]    hash function
 * @param key_cmp_fn [in] Keys comparison function
  *
 * @return pointer to new hashmap or NULL
 */
#define mdv_hashmap_create(type, key_field, capacity, hash_fn, key_cmp_fn)  \
    _mdv_hashmap_create(capacity,                                           \
                        offsetof(type, key_field),                          \
                        sizeof(((type*)0)->key_field),                      \
                        (mdv_hash_fn)&hash_fn,                              \
                        (mdv_key_cmp_fn)&key_cmp_fn)



/**
 * @brief Retains hashmap.
 * @details Reference counter is increased by one.
 */
mdv_hashmap * mdv_hashmap_retain(mdv_hashmap *hm);


/**
 * @brief Releases hashmap.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the hashmap's destructor is called.
 */
uint32_t mdv_hashmap_release(mdv_hashmap *hm);


/**
 * @brief Clear hash map.
 *
 * @param hm [in] Hash map allocated by mdv_hashmap()
 */
void mdv_hashmap_clear(mdv_hashmap *hm);


/**
 * @brief Return hash map size
 *
 * @param hm [in]         hash map
 *
 * @return hash map size
 */
size_t mdv_hashmap_size(mdv_hashmap const *hm);


/**
 * @brief Checks if the container has no elements
 */
#define mdv_hashmap_empty(hm) (mdv_hashmap_size(hm) == 0)


/**
 * @brief Return hash map capacity
 *
 * @param hm [in]         hash map
 *
 * @return hash map capacity
 */
size_t mdv_hashmap_capacity(mdv_hashmap const *hm);


/**
 * @brief Return hash map buckets
 *
 * @param hm [in]         hash map
 *
 * @return hash map buckets
 */
mdv_hashmap_bucket * mdv_hashmap_buckets(mdv_hashmap const *hm);


/**
 * @brief Resize hash map capacity.
 *
 * @param hm [in] Hash map allocated by mdv_hashmap()
 * @param capacity [in] New hash map capacity
 *
 * @return 1 if hash map resized
 * @return 0 if hash map wasn't resized
 */
bool mdv_hashmap_resize(mdv_hashmap *hm, size_t capacity);


/**
 * @brief Insert item into the hash map
 *
 * @param hm [in] Hash map allocated by mdv_hashmap()
 * @param item [in] New item
 *
 * @return nonzero pointer to new entry if item is inserted
 * @return 0 if no memory
 */
void * mdv_hashmap_insert(mdv_hashmap *hm, void const *item, size_t size);


/**
 * @brief Find item by key
 *
 * @param hm [in]           hash map allocated by mdv_hashmap()
 * @param key [in]          key
 *
 * @return On success, returns pointer to found entry
 * @return NULL if no entry found
 */
void * mdv_hashmap_find(mdv_hashmap const *hm, void const *key);


/**
 * @brief Remove value by key
 *
 * @param hm [in]           hash map allocated by mdv_hashmap()
 * @param key [in]          key
 *
 * @return true if hash item was removed
 * @return false if hash map wasn't removed
 */
bool mdv_hashmap_erase(mdv_hashmap *hm, void const *key);


/**
 * @brief hash map entries iterator
 *
 * @param hm [in]           hash map allocated by mdv_hashmap()
 * @param type [in]         hash map type
 * @param entry [out]       hash map entry
 *
 * Usage example:
 * @code
 *   mdv_hashmap_foreach(hm, type, entry)
 *   {
 *       type *item = entry;
 *   }
 * @endcode
 */
#define mdv_hashmap_foreach(hm, type, entry)                                    \
    for(size_t hm_idx__ = 0; hm_idx__ < mdv_hashmap_capacity(hm); ++hm_idx__)   \
        mdv_list_foreach(mdv_hashmap_buckets(hm) + hm_idx__, type, entry)
