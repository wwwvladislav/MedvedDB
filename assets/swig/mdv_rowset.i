%module mdv

%inline %{
#include <mdv_rowset.h>
#include <mdv_alloc.h>
%}

%include "mdv_row.i"

%{
typedef struct
{
    uint32_t        columns;
    mdv_enumerator *enumerator;
} mdv_rows_enumerator;
%}

%rename(RowSet)           mdv_rowset;
%rename(RowSetEnumerator) mdv_rows_enumerator;
%rename(enumerator)       get_enumerator;

%nodefault;
typedef struct {} mdv_rowset;
typedef struct {} mdv_rows_enumerator;
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

    mdv_rows_enumerator * get_enumerator()
    {
        mdv_rows_enumerator *enumerator = mdv_alloc(sizeof(mdv_rows_enumerator), "rows_enumerator");

        if(!enumerator)
            return 0;

        enumerator->columns = mdv_rowset_columns($self);
        enumerator->enumerator = mdv_rowset_enumerator($self);

        if (!enumerator->enumerator)
        {
            mdv_free(enumerator, "rows_enumerator");
            return 0;
        }

        return enumerator;
    }
}

%extend mdv_rows_enumerator
{
    ~mdv_rows_enumerator()
    {
        mdv_enumerator_release($self->enumerator);
        mdv_free($self, "rows_enumerator");
    }

    bool reset()
    {
        return mdv_enumerator_reset($self->enumerator) == MDV_OK;
    }

    bool next()
    {
        return mdv_enumerator_next($self->enumerator) == MDV_OK;
    }

    mdv_datums * current()
    {
        mdv_row *row = mdv_enumerator_current($self->enumerator);
        mdv_datums *datums = new_mdv_datums($self->columns);

        if (datums)
        {
            for(uint32_t i = 0; i < $self->columns; ++i)
            {
                datums->data[i] = mdv_datum_create(row->fields + i, &mdv_default_allocator);

                if (!datums->data[i])
                {
                    for(uint32_t j = 0; j < i; ++j)
                        mdv_datum_free(datums->data[j]);
                    delete_mdv_datums(datums);
                    return 0;
                }
            }
        }

        return datums;
    }
}
