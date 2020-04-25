#include "mdv_lrucache.h"
#include "mdv_alloc.h"
#include "mdv_log.h"
#include "mdv_list.h"
#include "mdv_hashmap.h"
#include "mdv_rollbacker.h"
#include <stdatomic.h>


struct mdv_lrucache
{
    atomic_uint_fast32_t    rc;
    size_t                  capacity;
    mdv_hashmap            *keys;       ///< hashmap<mdv_lrumap_entry>
    mdv_list                values;
};


typedef mdv_list_entry(mdv_lruitem) mdv_lrulist_entry;


typedef struct
{
    mdv_lrukey           key;
    mdv_lrulist_entry   *entry;
} mdv_lrumap_entry;


static size_t mdv_lrukey_hash(mdv_lrukey const *key)
{
    return *key;
}


static int mdv_lrukey_cmp(mdv_lrukey const *a, mdv_lrukey const *b)
{
    return *a > *b ? 1:
            *a < *b ? -1
            : 0;
}


mdv_lrucache * mdv_lrucache_create(size_t capacity)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    mdv_lrucache *cache = mdv_alloc(sizeof(mdv_lrucache), "lru_cache");

    if (!cache)
    {
        MDV_LOGE("No memory for LRU cache");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, cache, "lru_cache");

    atomic_init(&cache->rc, 1);

    cache->capacity = capacity;

    cache->keys = mdv_hashmap_create(mdv_lrumap_entry,
                                     key,
                                     capacity,
                                     mdv_lrukey_hash,
                                     mdv_lrukey_cmp);

    if(!cache->keys)
    {
        MDV_LOGE("No memory for LRU cache");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, cache->keys);

    memset(&cache->values, 0, sizeof cache->values);

    mdv_rollbacker_free(rollbacker);

    return cache;
}


mdv_lrucache * mdv_lrucache_retain(mdv_lrucache *cache)
{
    atomic_fetch_add_explicit(&cache->rc, 1, memory_order_acquire);
    return cache;
}


static void mdv_lrucache_free(mdv_lrucache *cache)
{
    mdv_hashmap_release(cache->keys);
    mdv_list_clear(&cache->values);
    mdv_free(cache, "lru_cache");
}


uint32_t mdv_lrucache_release(mdv_lrucache *cache)
{
    uint32_t rc = 0;

    if (cache)
    {
        rc = atomic_fetch_sub_explicit(&cache->rc, 1, memory_order_release) - 1;
        if(!rc)
            mdv_lrucache_free(cache);
    }

    return rc;
}


bool mdv_lrucache_put(mdv_lrucache *cache, mdv_lruitem new_item, mdv_lruitem *old)
{
    static const mdv_lruitem null_item = {};

    *old = null_item;

    mdv_lrukey key = new_item.key;
    void *data = new_item.data;

    mdv_lrumap_entry *item = mdv_hashmap_find(cache->keys, &key);

    if(item)
    {
        mdv_list_exclude(&cache->values, (mdv_list_entry_base *)item->entry);
        mdv_list_emplace_back(&cache->values, (mdv_list_entry_base *)item->entry);

        if(item->entry->data.data != data)
        {
            *old = (mdv_lruitem) { .key = key, .data = item->entry->data.data };
            item->entry->data.data = data;
        }

        return true;
    }

    if(mdv_hashmap_size(cache->keys) < cache->capacity)
    {
        mdv_lrulist_entry *lrulist_entry = mdv_alloc(sizeof(mdv_lrulist_entry), "lrulist_entry");

        if(!lrulist_entry)
        {
            MDV_LOGE("No memory for lrulist_entry");
            return false;
        }

        lrulist_entry->data.key = key;
        lrulist_entry->data.data = data;

        mdv_lrumap_entry lrumap_entry =
        {
            .key = key,
            .entry = lrulist_entry
        };

        if (!mdv_hashmap_insert(cache->keys, &lrumap_entry, sizeof lrumap_entry))
        {
            MDV_LOGE("No memory for lrumap_entry");
            mdv_free(lrulist_entry, "lrulist_entry");
            return false;
        }

        mdv_list_emplace_back(&cache->values, (mdv_list_entry_base *)lrulist_entry);
    }
    else
    {
        mdv_lrumap_entry new_lrumap_entry =
        {
            .key = key,
            .entry = 0
        };

        mdv_lrumap_entry *lrumap_entry = mdv_hashmap_insert(cache->keys, &new_lrumap_entry, sizeof new_lrumap_entry);

        if (!lrumap_entry)
        {
            MDV_LOGE("No memory for lrumap_entry");
            return false;
        }

        mdv_lrulist_entry *lrulist_entry = (mdv_lrulist_entry*)cache->values.next;

        *old = (mdv_lruitem) { .key = lrulist_entry->data.key, .data = lrulist_entry->data.data };

        lrulist_entry->data.key = key;
        lrulist_entry->data.data = data;

        lrumap_entry->entry = lrulist_entry;

        mdv_list_exclude(&cache->values, (mdv_list_entry_base *)lrulist_entry);
        mdv_list_emplace_back(&cache->values, (mdv_list_entry_base *)lrulist_entry);
        mdv_hashmap_erase(cache->keys, &old->key);
    }

    return true;
}


void * mdv_lrucache_get(mdv_lrucache *cache, mdv_lrukey key)
{
    mdv_lrumap_entry *item = mdv_hashmap_find(cache->keys, &key);

    if (!item)
        return 0;

    mdv_list_exclude(&cache->values, (mdv_list_entry_base *)item->entry);
    mdv_list_emplace_back(&cache->values, (mdv_list_entry_base *)item->entry);

    return item->entry->data.data;
}
