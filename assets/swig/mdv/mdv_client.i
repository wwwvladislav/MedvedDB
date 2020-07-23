%module mdv

%inline %{
#include <mdv_client.h>
%}

%rename(Client)                 mdv_client;

%nodefault;
typedef struct {} mdv_client;
%clearnodefault;

%include "mdv_table.i"
%include "mdv_rowset.i"
%include "mdv_client_config.i"
%include "mdv_bitset.i"

%newobject mdv_client::createTable;
%newobject mdv_client::getTable;
%newobject mdv_client::select;

%extend mdv_client
{
    static mdv_client * connect(mdv_client_config const *config);

    void close();

    mdv_table * createTable(mdv_table_desc *table)
    {
        return mdv_create_table($self, table);
    }

    mdv_table * getTable(mdv_uuid const *uuid)
    {
        return mdv_get_table($self, uuid);
    }

    bool insert(mdv_rowset *rowset)
    {
        return mdv_insert($self, rowset) == MDV_OK;
    }

    mdv_rowset * select(mdv_table *table, mdv_bitset *fields, char const *filter)
    {
        return mdv_select($self, table, fields, filter);
    }
}
