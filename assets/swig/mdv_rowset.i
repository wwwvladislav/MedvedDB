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

    bool add(mdv_datums const *row)
    {
        if(row->size != mdv_rowset_columns($self))
            return false;

        mdv_data data[row->size];

        for(uint32_t i = 0; i < row->size; ++i)
        {
            data[i].size = mdv_datum_size(row->data[i]);
            data[i].ptr = mdv_datum_ptr(row->data[i]);
        }

        mdv_data const *rows[] = { data };

        return mdv_rowset_append($self, rows, 1) == 1;
    }
}
