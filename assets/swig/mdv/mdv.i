%module mdv

%inline %{
#include <mdv_client.h>
%}

%include "mdv_client.i"
%include "mdv_uuid.i"

%rename(clientInitialize)       mdv_initialize;
%rename(clientFinalize)         mdv_finalize;

bool mdv_initialize();
void mdv_finalize();
