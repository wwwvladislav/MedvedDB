%module mdv

%inline %{
#include <mdv_table_desc.h>
%}

%include "stdint.i"
%include "mdv_field.i"

%rename(TableDesc)      mdv_table_desc;

%ignore mdv_table_desc::mdv_table_desc();

%immutable mdv_table_desc::name;
%ignore mdv_table_desc::size;
%ignore mdv_table_desc::fields;


%extend mdv_table_desc
{
    mdv_table_desc(char const *tableName)
    {
        return mdv_table_desc_create(tableName);
    }

    ~mdv_table_desc()
    {
        mdv_table_desc_free($self);
    }

    bool addField(mdv_field_type type, uint32_t limit, char const *name)
    {
        mdv_field const field =
        {
            .type = type,
            .limit = limit,
            .name = name
        };

        return mdv_table_desc_append($self, &field);
    }
}


%include "mdv_table_desc.h"
