/**
 * @file mdv_btree.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Btree
 * @version 0.1
 * @date 2021-03-25
 *
 * @copyright Copyright (c) 2021, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_def.h>
#include <mdv_macros.h>
#include <mdv_functional.h>


typedef uint32_t mdv_pageid_t;


typedef struct mdv_page
{
    mdv_pageid_t id;
    uint8_t data[1];
} mdv_page;


typedef struct
{
    mdv_page * (*alloc)(size_t size);
    void       (*free)(mdv_pageid_t);
    mdv_page * (*retain)(mdv_pageid_t);
    void       (*release)(mdv_pageid_t);
} mdv_btree_allocator;


/// BTree
typedef struct mdv_btree mdv_btree;


/// @cond Doxygen_Suppress


mdv_btree * _mdv_btree_create(uint32_t      order,
                              uint32_t      key_size,
                              uint32_t      item_size,
                              uint32_t      key_offset,
                              mdv_cmp_fn    key_cmp_fn);


/// @endcond


/**
 * @brief Create btree.
 *
 * @details Allocates space for btree and initializes fields.
 *
 * @param order [in]      btree order
 * @param type [in]       items type
 * @param key_field [in]  key field name in type
 * @param key_cmp_fn [in] Keys comparison function
  *
 * @return pointer to new btree or NULL
 */
#define mdv_btree_create(order, type, key_field, key_cmp_fn)            \
    _mdv_btree_create(order,                                            \
                      mdv_sizeof_member(type, key_field),               \
                      sizeof(type),                                     \
                      offsetof(type, key_field),                        \
                      (mdv_cmp_fn)&key_cmp_fn)


/**
 * @brief Retains btree.
 * @details Reference counter is increased by one.
 */
mdv_btree * mdv_btree_retain(mdv_btree *btree);


/**
 * @brief Releases btree.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the btree's destructor is called.
 */
uint32_t mdv_btree_release(mdv_btree *btree);


/**
 * @brief Clear btree.
 *
 * @param btree [in] Btree created by mdv_btree_create()
 */
void mdv_btree_clear(mdv_btree *btree);


/**
 * @brief Returns items count stored in btree
 *
 * @param btree [in]    Btree
 *
 * @return btree size
 */
size_t mdv_btree_size(mdv_btree const *btree);


/**
 * @brief Checks if the container has no elements
 */
#define mdv_btree_empty(btree) (mdv_btree_size(btree) == 0)


/**
 * @brief Insert item into the btree
 *
 * @param btree [in]    Btree
 * @param item [in]     New item to be inserted
 *
 * @return nonzero pointer to new entry if item is inserted
 * @return 0 if no memory
 */
void const * mdv_btree_insert(mdv_btree *btree, void const *item);


/**
 * @brief Find item by key
 *
 * @param btree [in]    Btree
 * @param key [in]      key
 *
 * @return On success, returns pointer to found entry
 * @return NULL if no entry found
 */
void const * mdv_btree_find(mdv_btree const *btree, void const *key);


/**
 * @brief Remove value by key
 *
 * @param btree [in]    Btree
 * @param key [in]      key
 *
 * @return true if item was removed
 * @return false if item wasn't removed
 */
bool mdv_btree_erase(mdv_btree *btree, void const *key);


/**
 * @brief btree entries iterator
 *
 * @param btree [in]    Btree
 * @param type [in]     Btree item type
 * @param entry [out]   Btree entry
 *
 * Usage example:
 * @code
 *   mdv_btree_foreach(btree, type, entry)
 *   {
 *       type *item = entry;
 *   }
 * @endcode
 */
#define mdv_btree_foreach(btree, type, entry)
