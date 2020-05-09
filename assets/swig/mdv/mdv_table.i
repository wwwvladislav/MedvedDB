%module mdv

%inline %{
#include <mdv_table.h>
%}

%include "mdv_table_desc.i"

%rename(Table) mdv_table;

%nodefault;
typedef struct {} mdv_table;
%clearnodefault;

%extend mdv_table
{
    ~mdv_table()
    {
        mdv_table_release($self);
    }
}
