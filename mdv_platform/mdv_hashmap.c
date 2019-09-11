#include "mdv_hashmap.h"
#include "mdv_alloc.h"
#include "mdv_log.h"
#include <string.h>


/// Hash map load factor (75%)
#define MDV_HASHMAP_LOAD_FACTOR 3 / 4


int _mdv_hashmap_init(mdv_hashmap   *hm,
                      size_t         capacity,
                      uint32_t       key_offset,
                      uint32_t       key_size,
                      mdv_hash_fn    hash_fn,
                      mdv_key_cmp_fn key_cmp_fn)
{
    hm->capacity    = capacity;
    hm->size        = 0;
    hm->key_offset  = key_offset;
    hm->key_size    = key_size;
    hm->hash_fn     = hash_fn;
    hm->key_cmp_fn  = key_cmp_fn;
    hm->buckets     = mdv_alloc(capacity * sizeof(mdv_hashmap_bucket), "hashmap.buckets");

    if (!hm->buckets)
    {
        MDV_LOGE("No memory for new hash map (capacity: %zu)", capacity);
        return 0;
    }

    memset(hm->buckets, 0, capacity * sizeof(mdv_hashmap_bucket));

    return 1;
}


void _mdv_hashmap_free(mdv_hashmap *hm)
{
    if (hm->buckets)
    {
        _mdv_hashmap_clear(hm);
        mdv_free(hm->buckets, "hashmap.buckets");
    }
}


void _mdv_hashmap_clear(mdv_hashmap *hm)
{
    if(hm->size)
    {
        for(size_t i = 0; i < hm->capacity; ++i)
            mdv_list_clear(hm->buckets[i]);
    }
}


int _mdv_hashmap_resize(mdv_hashmap *hm, size_t capacity)
{
    if (capacity >= hm->size)
    {
        mdv_hashmap_bucket *buckets = hm->buckets;

        hm->buckets = mdv_alloc(capacity * sizeof(mdv_hashmap_bucket), "hashmap.buckets");

        if (hm->buckets)
        {
            memset(hm->buckets, 0, capacity * sizeof(mdv_hashmap_bucket));

            for(size_t i = 0; i < hm->capacity; ++i)
            {
                for(mdv_list_entry_base *entry = buckets[i].next; entry; )
                {
                    mdv_list_entry_base *next = entry->next;

                    size_t const bucket_idx = hm->hash_fn(entry->data + hm->key_offset) % capacity;

                    mdv_list_emplace_back(hm->buckets[bucket_idx], entry);

                    entry = next;
                }
            }

            mdv_free(buckets, "hashmap.buckets");
        }
        else
        {
            MDV_LOGE("No memory for hash map resizing (new capacity: %zu)", capacity);
            hm->buckets = buckets;
            return 0;
        }

        hm->capacity = capacity;

        return 1;
    }

    return 0;
}


mdv_list_entry_base * _mdv_hashmap_insert(mdv_hashmap *hm, void const *item, size_t size)
{
    if (size < hm->key_offset + hm->key_size)
    {
        MDV_LOGE("New item is too small: %zu", size);
        return 0;
    }

    if (hm->size > hm->capacity * MDV_HASHMAP_LOAD_FACTOR)
    {
        if (!_mdv_hashmap_resize(hm, hm->capacity * 2))
            return 0;
    }

    void const *key = (char const *)item + hm->key_offset;

    size_t const bucket_idx = hm->hash_fn(key) % hm->capacity;

    mdv_hashmap_bucket *bucket = hm->buckets + bucket_idx;

    for(mdv_list_entry_base *entry = bucket->next; entry; entry = entry->next)
    {
        if(hm->key_cmp_fn(key, entry->data + hm->key_offset) == 0)
        {
            // Remove old value
            mdv_list_remove(*bucket, entry);
            hm->size--;
            break;
        }
    }

    // Insert new value
    mdv_list_entry_base * entry = mdv_list_push_back_ptr(*bucket, item, size);

    if (entry)
    {
        hm->size++;
        return entry;
    }

    return 0;
}


void * _mdv_hashmap_find(mdv_hashmap const *hm, void const *key)
{
    size_t const bucket_idx = hm->hash_fn(key) % hm->capacity;

    mdv_hashmap_bucket * bucket = hm->buckets + bucket_idx;

    for(mdv_list_entry_base *entry = bucket->next; entry; entry = entry->next)
    {
        if(hm->key_cmp_fn(key, entry->data + hm->key_offset) == 0)
            return entry->data;
    }

    return 0;
}


int _mdv_hashmap_erase(mdv_hashmap *hm, void const *key)
{
    size_t const bucket_idx = hm->hash_fn(key) % hm->capacity;

    mdv_hashmap_bucket * bucket = hm->buckets + bucket_idx;

    for(mdv_list_entry_base *entry = bucket->next; entry; entry = entry->next)
    {
        if(hm->key_cmp_fn(key, entry->data + hm->key_offset) == 0)
        {
            mdv_list_remove(*bucket, entry);
            hm->size--;
            return 1;
        }
    }

    return 0;
}
