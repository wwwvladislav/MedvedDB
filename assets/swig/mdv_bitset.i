%module mdv

%inline %{
#include <mdv_bitset.h>
%}

%include "stdint.i"

%rename(BitSet)         mdv_bitset;

%nodefault;
typedef struct mdv_bitset {} mdv_bitset;
%clearnodefault;

%extend mdv_bitset
{
    mdv_bitset(size_t size)
    {
        return mdv_bitset_create(size, &mdv_default_allocator);
    }

    ~mdv_bitset()
    {
        mdv_bitset_release($self);
    }

    bool set(size_t pos);
    void reset(size_t pos);
    bool test(size_t pos);
    void fill(bool val);
    size_t size();
    size_t count(bool val);
    size_t capacity();
}
