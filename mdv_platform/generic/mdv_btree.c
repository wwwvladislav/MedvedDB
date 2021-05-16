#include "mdv_btree.h"
#include "mdv_alloc.h"
#include "mdv_algorithm.h"
#include <mdv_def.h>
#include <mdv_log.h>
#include <string.h>
#include <stdatomic.h>
#include <assert.h>


typedef struct mdv_btree_node
{
    bool           leaf;                                    ///< leaf contains only keys without data
    uint32_t       size;                                    ///< items number in node
    mdv_pageid_t   id;                                      ///< current btree node identifier
    mdv_pageid_t   parent;                                  ///< parent identifier
    mdv_pageid_t  *child;                                   ///< child identifiers
    uint8_t       *keys;                                    ///< keys array
    uint8_t       *data;                                    ///< pointer to data array (optional)
} mdv_btree_node;


struct mdv_btree
{
    atomic_uint_fast32_t rc;                                ///< References counter
    size_t               size;                              ///< Items number stored in btree
    uint32_t             order;                             ///< Btree order
    uint32_t             key_size;                          ///< Key size
    uint32_t             item_size;                         ///< Item size
    uint32_t             key_offset;                        ///< key offset inside btree value
    mdv_btree_allocator *allocator;                         ///< Pages allocator
    mdv_cmp_fn           key_cmp_fn;                        ///< Keys comparison function
    mdv_btree_node      *root;                              ///< Root node
};


inline static uint32_t mdv_btree_keys_min(mdv_btree const *btree)
{
    return btree->order - 1;
}


inline static uint32_t mdv_btree_keys_max(mdv_btree const *btree)
{
    return 2 * btree->order - 1;
}


mdv_btree * _mdv_btree_create(uint32_t             order,
                              uint32_t             key_size,
                              uint32_t             item_size,
                              uint32_t             key_offset,
                              mdv_cmp_fn           key_cmp_fn,
                              mdv_btree_allocator *allocator)
{
    mdv_btree *btree = mdv_alloc(sizeof(mdv_btree), "btree");

    if (!btree)
    {
        MDV_LOGE("No memory for new btree");
        return 0;
    }

    atomic_init(&btree->rc, 1);

    btree->size         = 0;
    btree->order        = order;
    btree->key_size     = key_size;
    btree->item_size    = item_size;
    btree->key_offset   = key_offset;
    btree->allocator    = allocator->retain(allocator);
    btree->key_cmp_fn   = key_cmp_fn;
    btree->root         = 0;

    return btree;
}


static void mdv_btree_free(mdv_btree *btree)
{
    btree->allocator->release(btree->allocator);
    memset(btree, 0, sizeof *btree);
    mdv_free(btree, "btree");
}


mdv_btree * mdv_btree_retain(mdv_btree *btree)
{
    atomic_fetch_add_explicit(&btree->rc, 1, memory_order_acquire);
    return btree;
}


uint32_t mdv_btree_release(mdv_btree *btree)
{
    uint32_t rc = 0;

    if (btree)
    {
        rc = atomic_fetch_sub_explicit(&btree->rc, 1, memory_order_release) - 1;

        if (!rc)
            mdv_btree_free(btree);
    }

    return rc;
}


void mdv_btree_clear(mdv_btree *btree)
{
    // TODO
}


size_t mdv_btree_size(mdv_btree const *btree)
{
    return btree->size;
}


static mdv_btree_node * mdv_btree_find_leaf(mdv_btree const *btree, void const *key)
{
    mdv_btree_node *node = btree->root;

    if (node)
    {
        while (!node->leaf)
        {
            uint8_t const *pkey = mdv_lower_bound(key,
                                                  node->keys,
                                                  node->size,
                                                  btree->key_size,
                                                  btree->key_cmp_fn);

            assert(pkey && "Btree node shouldn't be empty");

            size_t const idx = (pkey - node->keys) / btree->key_size;

            int const diff = btree->key_cmp_fn(key, pkey);

            if (diff == 0)
            {
                mdv_pageid_t const child_id = node->child[idx + 1];
            }
            else
            {
                mdv_pageid_t const child_id = node->child[idx];
            }
        }
    }

    return node;
}


void const * mdv_btree_insert(mdv_btree *btree, void const *item)
{
    mdv_btree_node *leaf = mdv_btree_find_leaf(btree, (char const*)item + btree->key_offset);
    // TODO
    return 0;
}
