%module mdv

%inline %{
#include <mdv_rowset.h>
%}

%include "mdv_row.i"

%rename(RowSet) mdv_rowset;

%nodefault;
typedef struct {} mdv_rowset;
%clearnodefault;

%extend mdv_rowset
{
    mdv_rowset(uint32_t columns)
    {
        return mdv_rowset_create(columns);
    }

    ~mdv_rowset()
    {
        mdv_rowset_release($self);
    }

    uint32_t cols()
    {
        return mdv_rowset_columns($self);
    }

    bool add()
    {
        return true;
    }
}
