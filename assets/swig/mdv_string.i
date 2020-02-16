%module mdv

%inline %{
#include <mdv_string.h>
%}

%include "stdint.i"

%rename(String) mdv_string;

%include "mdv_string.h"

