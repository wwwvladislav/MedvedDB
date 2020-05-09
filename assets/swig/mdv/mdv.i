%module mdv

%inline %{
#include <mdv_client.h>
%}

%include "mdv_client.i"
%include "mdv_uuid.i"

%rename(clientInitialize)       mdv_initialize;
%rename(clientFinalize)         mdv_finalize;
%rename(threadInitialize)       mdv_thread_initialize;
%rename(threadFinalize)         mdv_thread_finalize;

bool mdv_initialize();
void mdv_finalize();
void mdv_thread_initialize();
void mdv_thread_finalize();
