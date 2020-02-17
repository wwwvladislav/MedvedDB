%module mdv

%inline %{
#include <mdv_bitset.h>
%}

%{
static mdv_bitset * mdv_bitset_create_impl(size_t size)
{
    return mdv_bitset_create(size, &mdv_default_allocator);
}
%}

%include "stdint.i"

%rename(BitSet)         mdv_bitset;
%rename(bitSetCreate)   mdv_bitset_create_impl;
%rename(bitSetRetain)   mdv_bitset_retain;
%rename(bitSetRelease)  mdv_bitset_release;
%rename(bitSetSet)      mdv_bitset_set;
%rename(bitSetReset)    mdv_bitset_reset;
%rename(bitSetTest)     mdv_bitset_test;
%rename(bitSetFill)     mdv_bitset_fill;
%rename(bitSetSize)     mdv_bitset_size;
%rename(bitSetCount)    mdv_bitset_count;
%rename(bitSetCapacity) mdv_bitset_capacity;

%nodefault;
typedef struct mdv_bitset {} mdv_bitset;
%clearnodefault;

mdv_bitset * mdv_bitset_create_impl(size_t size);
mdv_bitset * mdv_bitset_retain(mdv_bitset *bitset);
uint32_t     mdv_bitset_release(mdv_bitset *bitset);
bool         mdv_bitset_set(mdv_bitset *bitset, size_t pos);
void         mdv_bitset_reset(mdv_bitset *bitset, size_t pos);
bool         mdv_bitset_test(mdv_bitset const *bitset, size_t pos);
void         mdv_bitset_fill(mdv_bitset *bitset, bool val);
size_t       mdv_bitset_size(mdv_bitset const *bitset);
size_t       mdv_bitset_count(mdv_bitset const *bitset, bool val);
size_t       mdv_bitset_capacity(mdv_bitset const *bitset);
