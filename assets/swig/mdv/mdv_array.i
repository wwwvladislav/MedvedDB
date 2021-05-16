%module mdv

%inline %{
#include <mdv_alloc.h>
%}

%define %mdv_array(TYPE, NAME)
%{
typedef TYPE NAME;
%}

typedef struct {} NAME;

%extend NAME
{
    NAME(int size)                      { return mdv_alloc(sizeof(TYPE) * size); }
    ~NAME()                             { mdv_free(self); }
    TYPE get(size_t idx)                { return self[idx]; }
    void set(size_t idx, TYPE value)    { self[idx] = value; }
};

%types(NAME = TYPE);

%enddef
