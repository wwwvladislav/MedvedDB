#include "mdv_bitset.h"
#include "mdv_log.h"
#include <string.h>
#include <limits.h>


struct mdv_bitset
{
    mdv_allocator  const *allocator;
    size_t                size;
    uint32_t              bits[1];
};


static size_t mdv_bitset_capacity(size_t size)
{
    size_t const b = sizeof(uint32_t) * CHAR_BIT;
    return b * ((size + b - 1) / b);
}


mdv_bitset * mdv_bitset_create(size_t size, mdv_allocator const *allocator)
{
    mdv_bitset *bitset = allocator->alloc(offsetof(mdv_bitset, bits) + mdv_bitset_capacity(size) / CHAR_BIT, "bitset");

    if (!bitset)
    {
        MDV_LOGE("No memory for bitset");
        return 0;
    }

    bitset->allocator = allocator;
    bitset->size = size;

    memset(bitset->bits, 0, sizeof(mdv_bitset_capacity(size) / CHAR_BIT));

    return bitset;
}


void mdv_bitset_free(mdv_bitset *bitset)
{
    if (bitset)
        bitset->allocator->free(bitset, "bitset");
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


size_t mdv_bitset_size(mdv_bitset const *bitset)
{
    return bitset->size;
}
