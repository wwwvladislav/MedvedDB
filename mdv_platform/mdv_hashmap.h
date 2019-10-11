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
typedef struct mdv_hashmap
{
    size_t               capacity;                          ///< Hash map capacity
    size_t               size;                              ///< Items number stored in hash map
    uint32_t             key_offset;                        ///< key offset inside hash map value
    uint32_t             key_size;                          ///< key size
    mdv_hashmap_bucket  *buckets;                           ///< pointer to hash table
    mdv_hash_fn          hash_fn;                           ///< Hash function
    mdv_key_cmp_fn       key_cmp_fn;                        ///< Keys comparison function (used if hash collision happens)
} mdv_hashmap;


/// @cond Doxygen_Suppress


int                   _mdv_hashmap_init(mdv_hashmap   *hm,
                                        size_t         capacity,
                                        uint32_t       key_offset,
                                        uint32_t       key_size,
                                        mdv_hash_fn    hash_fn,
                                        mdv_key_cmp_fn key_cmp_fn);
void                  _mdv_hashmap_free(mdv_hashmap *hm);
void                  _mdv_hashmap_clear(mdv_hashmap *hm);
int                   _mdv_hashmap_resize(mdv_hashmap *hm, size_t capacity);
mdv_list_entry_base * _mdv_hashmap_insert(mdv_hashmap *hm, void const *item, size_t size);
void *                _mdv_hashmap_find(mdv_hashmap const *hm, void const *key);
int                   _mdv_hashmap_erase(mdv_hashmap *hm, void const *key);
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
 * @return 1 if hash map is initialized
 * @return 0 if hash map is not initialized
 */
#define mdv_hashmap_init(hm, type, key_field, capacity, hash_fn, key_cmp_fn)    \
    _mdv_hashmap_init(&(hm),                                                    \
                      capacity,                                                 \
                      offsetof(type, key_field),                                \
                      sizeof(((type*)0)->key_field),                            \
                      (mdv_hash_fn)&hash_fn,                                    \
                      (mdv_key_cmp_fn)&key_cmp_fn)


/**
 * @brief Free hash map.
 *
 * @param hm [in] Hash map allocated by mdv_hashmap()
 */
#define mdv_hashmap_free(hm)                                    \
    _mdv_hashmap_free(&(hm))


/**
 * @brief Clear hash map.
 *
 * @param hm [in] Hash map allocated by mdv_hashmap()
 */
#define mdv_hashmap_clear(hm)                                    \
    _mdv_hashmap_clear(&(hm))


/**
 * @brief Return hash map size
 *
 * @param hm [in]         hash map
 *
 * @return hash map size
 */
#define mdv_hashmap_size(hm) ((hm).size)


/**
 * @brief Resize hash map capacity.
 *
 * @param hm [in] Hash map allocated by mdv_hashmap()
 * @param capacity [in] New hash map capacity
 *
 * @return 1 if hash map resized
 * @return 0 if hash map wasn't resized
 */
#define mdv_hashmap_resize(hm, capacity)                        \
    _mdv_hashmap_resize(&(hm), capacity)


/**
 * @brief Insert item into the hash map
 *
 * @param hm [in] Hash map allocated by mdv_hashmap()
 * @param item [in] New item
 *
 * @return nonzero pointer to new entry if item is inserted
 * @return 0 if no memory
 */
#define mdv_hashmap_insert(hm, item)                            \
    _mdv_hashmap_insert(&(hm), &(item), sizeof(item))


/**
 * @brief Find item by key
 *
 * @param hm [in]           hash map allocated by mdv_hashmap()
 * @param key [in]          key
 *
 * @return On success, returns pointer to found entry
 * @return NULL if no entry found
 */
#define mdv_hashmap_find(hm, key)                               \
    _mdv_hashmap_find(&(hm), &(key))


/**
 * @brief Remove value by key
 *
 * @param hm [in]           hash map allocated by mdv_hashmap()
 * @param key [in]          key
 *
 * @return 1 if hash item was removed
 * @return 0 if hash map wasn't removed
 */
#define mdv_hashmap_erase(hm, key)                              \
    _mdv_hashmap_erase(&(hm), &(key))


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
#define mdv_hashmap_foreach(hm, type, entry)                        \
    for(size_t hm_idx__ = 0; hm_idx__ < (hm).capacity; ++hm_idx__)  \
        mdv_list_foreach((hm).buckets + hm_idx__, type, entry)
