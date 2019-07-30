#include "mdv_algorithm.h"


void mdv_union_and_diff_u32(uint32_t const *set_a, uint32_t a_size,
                            uint32_t const *set_b, uint32_t b_size,
                            uint32_t *sets_union, uint32_t *union_size,
                            uint32_t *sets_diff, uint32_t *diff_size)
{
    uint32_t *sets_union_begin = sets_union;
    uint32_t *sets_diff_begin = sets_diff;

    uint32_t i = 0, j = 0;

    while(i < a_size)
    {
        if (j >= b_size)
        {
            for(; i < a_size; ++i)
            {
                *(sets_diff++) = set_a[i];
                *(sets_union++) = set_a[i];
            }
            break;
        }

        if(set_a[i] < set_b[j])
        {
            *(sets_diff++) = set_a[i];
            *(sets_union++) = set_a[i];
            ++i;
        }
        else
        {
            *(sets_union++) = set_b[j];
            if (set_a[i] == set_b[j])
                ++i;
            else
                *(sets_union++) = set_a[i];
            ++j;
        }
    }

    for(; j < b_size; ++j)
        *(sets_union++) = set_b[j];

    *union_size = sets_union - sets_union_begin;
    *diff_size = sets_diff - sets_diff_begin;
}
