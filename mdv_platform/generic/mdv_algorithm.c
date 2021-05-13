#include "mdv_algorithm.h"
#include <string.h>
#include <stdlib.h>


void mdv_diff_u32(uint32_t const *set_a, uint32_t a_size,
                  uint32_t const *set_b, uint32_t b_size,
                  uint32_t *sets_diff, uint32_t *diff_size)
{
    uint32_t *sets_diff_begin = sets_diff;

    uint32_t ai = 0, bi = 0;

    while(ai < a_size)
    {
        if (bi >= b_size)
        {
            for(; ai < a_size; ++ai)
                *(sets_diff++) = set_a[ai];
            break;
        }

        if(set_a[ai] < set_b[bi])
            *(sets_diff++) = set_a[ai++];
        else
        {
            if (set_a[ai] == set_b[bi])
                ++ai;
            ++bi;
        }
    }

    *diff_size = sets_diff - sets_diff_begin;
}


void mdv_union_u32(uint32_t const *set_a, uint32_t a_size,
                   uint32_t const *set_b, uint32_t b_size,
                   uint32_t *sets_union, uint32_t *union_size)
{
    uint32_t *sets_union_begin = sets_union;

    uint32_t ai = 0, bi = 0;

    while(ai < a_size)
    {
        if (bi >= b_size)
        {
            for(; ai < a_size; ++ai)
                *(sets_union++) = set_a[ai];
            break;
        }

        if(set_b[bi] < set_a[ai])
            *(sets_union++) = set_b[bi++];
        else
        {
            *(sets_union++) = set_a[ai];
            if (set_a[ai] == set_b[bi])
                ++bi;
            ++ai;
        }
    }

    for(; bi < b_size; ++bi)
        *(sets_union++) = set_b[bi];

    *union_size = sets_union - sets_union_begin;
}


uint32_t mdv_exclude(void       *set_a, size_t itmsize_a, size_t size_a,
                     void const *set_b, size_t itmsize_b, size_t size_b,
                     mdv_cmp_fn  cmp)
{
    char const *set_a_begin = set_a;
    char const *set_a_end   = (char*)set_a + size_a * itmsize_a;
    char const *set_b_begin = set_b;
    char const *set_b_end   = (char*)set_b + size_b * itmsize_b;
    char *set_out           = set_a;

    while((char const*)set_a < set_a_end)
    {
        if ((char const*)set_b >= set_b_end)
        {
            for(; (char const *)set_a < set_a_end; set_a = (char*)set_a + itmsize_a)
            {
                memcpy(set_out, set_a, itmsize_a);
                set_out += itmsize_a;
            }
            break;
        }

        if(cmp(set_a, set_b) < 0)
        {
            memcpy(set_out, set_a, itmsize_a);
            set_out += itmsize_a;
            set_a = (char*)set_a + itmsize_a;
        }
        else
        {
            if (cmp(set_a, set_b) == 0)
                set_a = (char*)set_a + itmsize_a;
            set_b = (char*)set_b + itmsize_b;
        }
    }

    return (set_out - set_a_begin) / itmsize_a;
}


void *mdv_bsearch(const void *key,
                  const void *base,
                  size_t      nmemb,
                  size_t      size,
                  mdv_cmp_fn  cmp)
{
    return bsearch(key, base, nmemb, size, cmp);
}


void mdv_qsort(void       *base,
               size_t      nmemb,
               size_t      size,
               mdv_cmp_fn  cmp)
{
    qsort(base, nmemb, size, cmp);
}


void *mdv_lower_bound(const void *key,
                      const void *base,
                      size_t      nmemb,
                      size_t      size,
                      mdv_cmp_fn  cmp)
{
    if (!nmemb)
        return 0;

    char *arr = (char *)base;
    char *a = arr;
    char *b = arr + size * (nmemb - 1);

    while(b > a)
    {
        size_t const n = (b - a) / size;
        char *c = a + (n / 2) * size;

        int diff = cmp(key, c);

        if(diff < 0)
            b = c - size;
        else if(diff > 0)
            a = c + size;
        else
            return c;
    }

    return cmp(key, b) > 0
                ? b + size
                : b;
}
