#pragma once
#include <minunit.h>
#include <mdv_hashmap.h>


static size_t hashmap_hash(int const *v)
{
    return *v;
}


static int hashmap_keys_cmp(int const *a, int const *b)
{
    return *a - *b;
}


MU_TEST(platform_hashmap)
{
    typedef struct
    {
        int val;
        int key;
    } map_entry;

    mdv_hashmap *map = mdv_hashmap_create(map_entry, key, 5, hashmap_hash, hashmap_keys_cmp);

    mu_check(map);

    map_entry entry = {};

    mu_check(mdv_hashmap_insert(map, &entry, sizeof entry)); entry.key++; entry.val++;
    mu_check(mdv_hashmap_insert(map, &entry, sizeof entry)); entry.key++; entry.val++;
    mu_check(mdv_hashmap_insert(map, &entry, sizeof entry)); entry.key++; entry.val++;
    mu_check(mdv_hashmap_insert(map, &entry, sizeof entry)); entry.key++; entry.val++;
    mu_check(mdv_hashmap_insert(map, &entry, sizeof entry)); entry.key++; entry.val++;

    int s = 0;

    mdv_hashmap_foreach(map, map_entry, entry)
    {
        mu_check(entry->key == s && entry->val == s);
        ++s;
    }

    mu_check(s == 5);

    mu_check(mdv_hashmap_insert(map, &entry, sizeof entry)); entry.val = 42;
    mu_check(mdv_hashmap_insert(map, &entry, sizeof entry));

    mu_check(mdv_hashmap_size(map) == 6);

    int key = 5;

    map_entry * e = mdv_hashmap_find(map, &key);

    mu_check(e && e->val == 42);

    mu_check(mdv_hashmap_erase(map, &key));

    mu_check(mdv_hashmap_find(map, &key) == 0);

    mu_check(mdv_hashmap_size(map) == 5);

    mdv_hashmap_release(map);
}
