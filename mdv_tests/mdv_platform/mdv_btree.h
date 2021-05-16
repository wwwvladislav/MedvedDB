#pragma once
#include <minunit.h>
#include <mdv_btree.h>


typedef struct mdv_btree_test_allocator
{
    mdv_btree_allocator base;
    uint32_t rc;
} mdv_btree_test_allocator;


static mdv_btree_allocator * mdv_btree_test_allocator_retain(mdv_btree_allocator *allocator)
{
    mdv_btree_test_allocator *test_allocator = (mdv_btree_test_allocator *)allocator;
    test_allocator->rc++;
    return allocator;
}


static uint32_t mdv_btree_test_allocator_release(mdv_btree_allocator *allocator)
{
    mdv_btree_test_allocator *test_allocator = (mdv_btree_test_allocator *)allocator;
    uint32_t rc = --test_allocator->rc;
    if (!rc)
    {
        mdv_free(test_allocator, "mdv_btree_test_allocator");
    }
    return rc;
}


static mdv_page * mdv_btree_test_allocator_page_alloc(size_t size)
{
    (void)size;
    return 0;
}


static void mdv_btree_test_allocator_page_free(mdv_pageid_t id)
{
    (void)id;
}


static mdv_page * mdv_btree_test_allocator_page_retain(mdv_pageid_t id)
{
    (void)id;
    return 0;
}


static void mdv_btree_test_allocator_page_release(mdv_pageid_t id)
{
    (void)id;
}


static mdv_btree_test_allocator * mdv_btree_test_allocator_create()
{
    mdv_btree_test_allocator *allocator = mdv_alloc(sizeof(mdv_btree_test_allocator), "mdv_btree_test_allocator");
    allocator->base.retain = mdv_btree_test_allocator_retain;
    allocator->base.release = mdv_btree_test_allocator_release;
    allocator->base.page_alloc = mdv_btree_test_allocator_page_alloc;
    allocator->base.page_free = mdv_btree_test_allocator_page_free;
    allocator->base.page_retain = mdv_btree_test_allocator_page_retain;
    allocator->base.page_release = mdv_btree_test_allocator_page_release;
    allocator->rc = 1;
    return allocator;
}


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

    mdv_btree_test_allocator *allocator = mdv_btree_test_allocator_create();

    mdv_btree *btree = mdv_btree_create(3, btree_entry, key, btree_keys_cmp, &allocator->base);
    mu_check(btree);
    mu_check((void*)&allocator->base == (void*)allocator);

    allocator->base.release(&allocator->base);

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
