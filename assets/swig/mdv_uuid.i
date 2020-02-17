%module mdv

%inline %{
#include <mdv_uuid.h>
%}

%include "stdint.i"

%rename(UUID)         mdv_uuid;
%rename(uuidToStr)    mdv_uuid_to_str;

typedef struct {} mdv_uuid;

char const * mdv_uuid_to_str(mdv_uuid const *uuid);
