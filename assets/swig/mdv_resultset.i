%module mdv

%inline %{
#include <mdv_client.h>
%}

//%include "mdv_resultset_enumerator.i"

%rename(ResultSet) mdv_resultset;

%nodefault;
typedef struct {} mdv_resultset;
%clearnodefault;

%extend mdv_resultset
{
}

