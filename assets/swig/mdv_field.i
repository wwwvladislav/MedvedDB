%module mdv

%inline %{
#include <mdv_field.h>
%}

%include "stdint.i"

%rename(Field)              mdv_field;
%rename(FieldType)          mdv_field_type;
%rename(fieldTypeSize)      mdv_field_type_size;

%ignore mdv_field::mdv_field();
%ignore mdv_field::~mdv_field();

%immutable mdv_field::type;
%immutable mdv_field::limit;
%immutable mdv_field::name;

%include "mdv_field.h"
