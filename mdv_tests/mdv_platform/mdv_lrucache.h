#pragma once
#include <minunit.h>
#include <mdv_lrucache.h>


MU_TEST(platform_lrucache)
{
    mdv_lrucache *cache = mdv_lrucache_create(4);

    mdv_lruitem old;

    mu_check(mdv_lrucache_put(cache, (mdv_lruitem) { 1, (void*)1 }, &old));
    mu_check(mdv_lrucache_put(cache, (mdv_lruitem) { 2, (void*)2 }, &old));
    mu_check(mdv_lrucache_put(cache, (mdv_lruitem) { 3, (void*)3 }, &old));
    mu_check(mdv_lrucache_put(cache, (mdv_lruitem) { 4, (void*)4 }, &old));
    mu_check(mdv_lrucache_put(cache, (mdv_lruitem) { 5, (void*)5 }, &old));

    // 2, 3, 4, 5
    mu_check(old.key == 1 && old.data == (void*)1);

    mu_check(mdv_lrucache_put(cache, (mdv_lruitem) { 2, (void*)2 }, &old));

    // 3, 4, 5, 2
    mu_check(old.key == 0 && old.data == (void*)0);

    mu_check(mdv_lrucache_get(cache, 42) == 0);
    mu_check(mdv_lrucache_get(cache, 3) == (void*)3);

    // 4, 5, 2, 3
    mu_check(mdv_lrucache_put(cache, (mdv_lruitem) { 6, (void*)6 }, &old));
    mu_check(old.key == 4 && old.data == (void*)4);
    mu_check(mdv_lrucache_put(cache, (mdv_lruitem) { 7, (void*)7 }, &old));
    mu_check(old.key == 5 && old.data == (void*)5);
    mu_check(mdv_lrucache_put(cache, (mdv_lruitem) { 8, (void*)8 }, &old));
    mu_check(old.key == 2 && old.data == (void*)2);
    mu_check(mdv_lrucache_put(cache, (mdv_lruitem) { 9, (void*)9 }, &old));
    mu_check(old.key == 3 && old.data == (void*)3);

    mdv_lrucache_release(cache);
}
