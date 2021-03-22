#include "mdv_bitset.h"
#include <mdv_log.h>
#include <string.h>
#include <limits.h>
#include <stdatomic.h>


struct mdv_bitset
{
    atomic_uint_fast32_t  rc;                   ///< References counter
    mdv_allocator  const *allocator;            ///< Memory allocator
    size_t                size;                 ///< Bitset size
    uint32_t              bits[1];              ///< Bits vector
};


static size_t mdv_bitset_capacity_calc(size_t size)
{
    size_t const b = sizeof(uint32_t) * CHAR_BIT;
    return b * ((size + b - 1) / b);
}


mdv_bitset * mdv_bitset_create(size_t size, mdv_allocator const *allocator)
{
    mdv_bitset *bitset = allocator->alloc(offsetof(mdv_bitset, bits) + mdv_bitset_capacity_calc(size) / CHAR_BIT, "bitset");

    if (!bitset)
    {
        MDV_LOGE("No memory for bitset");
        return 0;
    }

    atomic_init(&bitset->rc, 1);

    bitset->allocator = allocator;
    bitset->size = size;

    mdv_bitset_fill(bitset, false);

    return bitset;
}


static void mdv_bitset_free(mdv_bitset *bitset)
{
    if (bitset)
        bitset->allocator->free(bitset, "bitset");
}


mdv_bitset * mdv_bitset_retain(mdv_bitset *bitset)
{
    atomic_fetch_add_explicit(&bitset->rc, 1, memory_order_acquire);
    return bitset;
}


uint32_t mdv_bitset_release(mdv_bitset *bitset)
{
    uint32_t rc = 0;

    if (bitset)
    {
        rc = atomic_fetch_sub_explicit(&bitset->rc, 1, memory_order_release) - 1;

        if (!rc)
            mdv_bitset_free(bitset);
    }

    return rc;
}


bool mdv_bitset_set(mdv_bitset *bitset, size_t pos)
{
    size_t const b = CHAR_BIT * sizeof *bitset->bits;
    size_t const n = pos / b;
    size_t const offs = pos % b;

    if ((bitset->bits[n] >> offs) & 1)
        return false;

    bitset->bits[n] |= 1u << offs;

    return true;
}


void mdv_bitset_reset(mdv_bitset *bitset, size_t pos)
{
    size_t const b = CHAR_BIT * sizeof *bitset->bits;
    size_t const n = pos / b;
    size_t const offs = pos % b;
    bitset->bits[n] &= ~(1u << offs);
}


bool mdv_bitset_test(mdv_bitset const *bitset, size_t pos)
{
    size_t const b = CHAR_BIT * sizeof *bitset->bits;
    size_t const n = pos / b;
    size_t const offs = pos % b;
    return (bitset->bits[n] >> offs) & 1;
}


void mdv_bitset_fill(mdv_bitset *bitset, bool val)
{
    memset(bitset->bits,
           val ? -1 : 0,
           mdv_bitset_capacity(bitset) / CHAR_BIT);
}


size_t mdv_bitset_size(mdv_bitset const *bitset)
{
    return bitset->size;
}


size_t mdv_bitset_count(mdv_bitset const *bitset, bool val)
{
    size_t n = 0;

    for(size_t i = 0; i < bitset->size; ++i)
        if (mdv_bitset_test(bitset, i) == val)
            ++n;

    return n;
}


size_t mdv_bitset_capacity(mdv_bitset const *bitset)
{
    return mdv_bitset_capacity_calc(bitset->size);
}


uint32_t const * mdv_bitset_data(mdv_bitset const *bitset)
{
    return bitset->bits;
}
