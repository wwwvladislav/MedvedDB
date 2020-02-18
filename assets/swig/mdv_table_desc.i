%module mdv

%inline %{
#include <mdv_table_desc.h>
%}

%include "stdint.i"
%include "carrays.i"
%include "mdv_field.i"

%array_functions(mdv_field, FieldsArray);

%rename(TableDesc)      mdv_table_desc;

%ignore mdv_table_desc::mdv_table_desc();

%immutable mdv_table_desc::name;
%immutable mdv_table_desc::fields;

/* TODO
%extend mdv_table_desc
{
    mdv_table_desc(char const *name, mdv_field const *fields, size_t count)
    {
        // Calculate space for srtings
        size_t const name_size = strlen(name) + 1;
        size_t str_space_size = name_size;

        for(size_t i = 0; i < count; ++i)
            str_space_size += strlen(fields[i].name) + 1;

        mdv_table_desc *desc = malloc(sizeof(mdv_table_desc)
                                        + sizeof(mdv_field) * count
                                        + str_space_size);

        if(!desc)
            return 0;

        mdv_field *pfields = (mdv_field *)(desc + 1);
        char *strings = (char *)(pfields + count);

        desc->name = strings;
        memcpy(strings, name, name_size);
        strings += name_size;

        return desc;
    }
}
*/

%include "mdv_table_desc.h"
