%module mdv

%inline %{
#include <mdv_uuid.h>
%}

%include "stdint.i"

%rename(UUID)         mdv_uuid;

typedef struct {} mdv_uuid;

