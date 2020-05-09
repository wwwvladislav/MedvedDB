%module mdv

%inline %{
#include <mdv_rowset.h>
#include <mdv_alloc.h>
%}

%include "mdv_row.i"
%include "mdv_table.i"

%{
typedef struct
{
    mdv_table      *table;
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

%newobject mdv_rowset::get_enumerator;

%extend mdv_rowset
{
    mdv_rowset(mdv_table *table)
    {
        return mdv_rowset_create(table);
    }

    ~mdv_rowset()
    {
        mdv_rowset_release($self);
    }

    bool add(mdv_datums const *row)
    {
        mdv_table *table = mdv_rowset_table($self);
        mdv_table_desc const *table_desc = mdv_table_description(table);

        if(row->size != table_desc->size)
        {
            // Invalid row length
            mdv_table_release(table);
            return false;
        }

        mdv_data data[row->size];

        for(uint32_t i = 0; i < row->size; ++i)
        {
            if(mdv_datum_type(row->data[i]) != table_desc->fields[i].type)
            {
                // Invalid field type
                mdv_table_release(table);
                return false;
            }

            data[i].size = mdv_datum_size(row->data[i]);
            data[i].ptr = mdv_datum_ptr(row->data[i]);
        }

        mdv_data const *rows[] = { data };

        mdv_table_release(table);

        return mdv_rowset_append($self, rows, 1) == 1;
    }

    mdv_rows_enumerator * get_enumerator()
    {
        mdv_rows_enumerator *enumerator = mdv_alloc(sizeof(mdv_rows_enumerator), "rows_enumerator");

        if(!enumerator)
            return 0;

        enumerator->enumerator = mdv_rowset_enumerator($self);

        if (!enumerator->enumerator)
        {
            mdv_free(enumerator, "rows_enumerator");
            return 0;
        }

        enumerator->table = mdv_rowset_table($self);

        return enumerator;
    }
}

%newobject mdv_rows_enumerator::current;

%extend mdv_rows_enumerator
{
    ~mdv_rows_enumerator()
    {
        mdv_table_release($self->table);
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
        mdv_row              *row    = mdv_enumerator_current($self->enumerator);
        mdv_table_desc const *desc   = mdv_table_description($self->table);
        mdv_datums           *datums = new_mdv_datums(desc->size);

        if (datums)
        {
            for(uint32_t i = 0; i < desc->size; ++i)
            {
                datums->data[i] = mdv_datum_create(desc->fields[i].type, row->fields[i].ptr, row->fields[i].size);

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
