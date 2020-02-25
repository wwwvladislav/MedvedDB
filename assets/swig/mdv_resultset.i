%module mdv

%inline %{
#include <mdv_resultset.h>
%}

%include "mdv_rowset.i"

%{
typedef mdv_enumerator mdv_result_enumerator;
%}

%rename(ResultSet)           mdv_resultset;
%rename(ResultSetEnumerator) mdv_result_enumerator;
%rename(enumerator)          get_enumerator;

%nodefault;
typedef struct {} mdv_resultset;
typedef struct {} mdv_result_enumerator;
%clearnodefault;

%extend mdv_resultset
{
    mdv_result_enumerator * get_enumerator()
    {
        return mdv_resultset_enumerator($self);
    }
}

%extend mdv_result_enumerator
{
    ~mdv_result_enumerator()
    {
        mdv_enumerator_release($self);
    }

    bool next()
    {
        return mdv_enumerator_next($self) == MDV_OK;
    }

    mdv_rowset * current()
    {
        return mdv_enumerator_current($self);
    }
}
