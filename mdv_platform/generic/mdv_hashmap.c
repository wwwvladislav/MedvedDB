#include "mdv_hashmap.h"
#include "mdv_alloc.h"
#include <mdv_log.h>
#include <string.h>
#include <stdatomic.h>


/// Hash map
struct mdv_hashmap
{
    atomic_uint_fast32_t rc;                                ///< References counter
    size_t               capacity;                          ///< Hash map capacity
    size_t               size;                              ///< Items number stored in hash map
    uint32_t             key_offset;                        ///< key offset inside hash map value
    uint32_t             key_size;                          ///< key size
    mdv_hashmap_bucket  *buckets;                           ///< pointer to hash table
    mdv_hash_fn          hash_fn;                           ///< Hash function
    mdv_cmp_fn           key_cmp_fn;                        ///< Keys comparison function (used if hash collision happens)
};


/// Hash map load factor (75%)
#define MDV_HASHMAP_LOAD_FACTOR 3 / 4


mdv_hashmap * _mdv_hashmap_create(size_t        capacity,
                                  uint32_t      key_offset,
                                  uint32_t      key_size,
                                  mdv_hash_fn   hash_fn,
                                  mdv_cmp_fn    key_cmp_fn)
{
    mdv_hashmap *hm = mdv_alloc(sizeof(mdv_hashmap));

    if (!hm)
    {
        MDV_LOGE("No memory for new hashmap");
        return 0;
    }

    atomic_init(&hm->rc, 1);

    capacity *= 5 / 4;

    hm->capacity    = capacity;
    hm->size        = 0;
    hm->key_offset  = key_offset;
    hm->key_size    = key_size;
    hm->hash_fn     = hash_fn;
    hm->key_cmp_fn  = key_cmp_fn;

    if (capacity)
    {
        hm->buckets = mdv_alloc(capacity * sizeof(mdv_hashmap_bucket));

        if (!hm->buckets)
        {
            MDV_LOGE("No memory for new hash map (capacity: %zu)", capacity);
            mdv_free(hm);
            return 0;
        }

        memset(hm->buckets, 0, capacity * sizeof(mdv_hashmap_bucket));
    }
    else
        hm->buckets = 0;

    return hm;
}


static void mdv_hashmap_free(mdv_hashmap *hm)
{
    if (hm->buckets)
    {
        mdv_hashmap_clear(hm);
        mdv_free(hm->buckets);
    }
    memset(hm, 0, sizeof *hm);
    mdv_free(hm);
}


mdv_hashmap * mdv_hashmap_retain(mdv_hashmap *hm)
{
    atomic_fetch_add_explicit(&hm->rc, 1, memory_order_acquire);
    return hm;
}


uint32_t mdv_hashmap_release(mdv_hashmap *hm)
{
    uint32_t rc = 0;

    if (hm)
    {
        rc = atomic_fetch_sub_explicit(&hm->rc, 1, memory_order_release) - 1;

        if (!rc)
            mdv_hashmap_free(hm);
    }

    return rc;
}


void mdv_hashmap_clear(mdv_hashmap *hm)
{
    if(hm->size)
    {
        for(size_t i = 0; i < hm->capacity; ++i)
            mdv_list_clear(hm->buckets + i);
    }
}


size_t mdv_hashmap_size(mdv_hashmap const *hm)
{
    return hm->size;
}


size_t mdv_hashmap_capacity(mdv_hashmap const *hm)
{
    return hm->capacity;
}


mdv_hashmap_bucket * mdv_hashmap_buckets(mdv_hashmap const *hm)
{
    return hm->buckets;
}


bool mdv_hashmap_resize(mdv_hashmap *hm, size_t capacity)
{
    if (capacity > hm->size)
    {
        mdv_hashmap_bucket *buckets = hm->buckets;

        hm->buckets = mdv_alloc(capacity * sizeof(mdv_hashmap_bucket));

        if (hm->buckets)
        {
            memset(hm->buckets, 0, capacity * sizeof(mdv_hashmap_bucket));

            for(size_t i = 0; i < hm->capacity; ++i)
            {
                for(mdv_list_entry_base *entry = buckets[i].next; entry; )
                {
                    mdv_list_entry_base *next = entry->next;

                    size_t const bucket_idx = hm->hash_fn(entry->data + hm->key_offset) % capacity;

                    mdv_list_emplace_back(hm->buckets + bucket_idx, entry);

                    entry = next;
                }
            }

            mdv_free(buckets);
        }
        else
        {
            MDV_LOGE("No memory for hash map resizing (new capacity: %zu)", capacity);
            hm->buckets = buckets;
            return false;
        }

        hm->capacity = capacity;

        return true;
    }

    return 0;
}


void * mdv_hashmap_insert(mdv_hashmap *hm, void const *item, size_t size)
{
    if (size < hm->key_offset + hm->key_size)
    {
        MDV_LOGE("New item is too small: %zu", size);
        return 0;
    }

    if (hm->size > hm->capacity * MDV_HASHMAP_LOAD_FACTOR)
    {
        if (!mdv_hashmap_resize(hm, hm->capacity * 2))
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
            mdv_list_remove(bucket, entry);
            hm->size--;
            break;
        }
    }

    // Insert new value
    mdv_list_entry_base * entry = mdv_list_push_back_data(bucket, item, size);

    if (entry)
    {
        hm->size++;
        return entry->data;
    }

    return 0;
}


void * mdv_hashmap_find(mdv_hashmap const *hm, void const *key)
{
    if (!hm->capacity)
        return 0;

    size_t const bucket_idx = hm->hash_fn(key) % hm->capacity;

    mdv_hashmap_bucket * bucket = hm->buckets + bucket_idx;

    for(mdv_list_entry_base *entry = bucket->next; entry; entry = entry->next)
    {
        if(hm->key_cmp_fn(key, entry->data + hm->key_offset) == 0)
            return entry->data;
    }

    return 0;
}


bool mdv_hashmap_erase(mdv_hashmap *hm, void const *key)
{
    if (!hm->capacity)
        return false;

    size_t const bucket_idx = hm->hash_fn(key) % hm->capacity;

    mdv_hashmap_bucket * bucket = hm->buckets + bucket_idx;

    for(mdv_list_entry_base *entry = bucket->next; entry; entry = entry->next)
    {
        if(hm->key_cmp_fn(key, entry->data + hm->key_offset) == 0)
        {
            mdv_list_remove(bucket, entry);
            hm->size--;
            return true;
        }
    }

    return false;
}
