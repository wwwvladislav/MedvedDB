%module mdv

%inline %{
#include <mdv_field.h>
%}

%include "stdint.i"

%rename(Field)              mdv_field;
%rename(FieldType)          mdv_field_type;
%rename(fieldTypeSize)      mdv_field_type_size;

%include "mdv_field.h"
