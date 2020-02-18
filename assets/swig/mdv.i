%module mdv

%inline %{
#include <mdv_client.h>
%}

%include "stdint.i"

%include "mdv_client.i"
%include "mdv_uuid.i"
%include "mdv_bitset.i"
%include "mdv_table_desc.i"
