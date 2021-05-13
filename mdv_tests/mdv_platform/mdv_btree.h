#pragma once
#include <minunit.h>
#include <mdv_btree.h>


static int btree_keys_cmp(int const *a, int const *b)
{
    return *a - *b;
}


MU_TEST(platform_btree)
{
    typedef struct btree_entry
    {
        int key;
        int val;
    } btree_entry;

    mdv_btree *btree = mdv_btree_create(3, btree_entry, key, btree_keys_cmp);
    mu_check(btree);

    mu_check(mdv_btree_empty(btree));

    btree_entry const values[] =
    {
        { 1, -1 },
        { 2, -2 },
        { 3, -3 },
    };

    for (unsigned int i = 0; i < sizeof values / sizeof *values; ++i)
    {
        btree_entry const *entry = mdv_btree_insert(btree, values + i);

//        mu_check(entry != 0);
//        mu_check(entry->key == values[i].key);
//        mu_check(entry->val == values[i].val);
    }

    mu_check(mdv_btree_retain(btree));
    mu_check(mdv_btree_release(btree) == 1);

    mu_check(mdv_btree_release(btree) == 0);
}
