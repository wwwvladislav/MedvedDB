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

mdv_table * mdv_create_table(mdv_client *client, mdv_table_desc *table);

%newobject mdv_client::createTable;
%newobject mdv_client::select;

%extend mdv_client
{
    static mdv_client * connect(mdv_client_config const *config);

    void close();

    mdv_table * createTable(mdv_table_desc *table)
    {
        return mdv_create_table($self, table);
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
