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

mdv_table *  mdv_create_table(mdv_client *client, mdv_table_desc *table);

%extend mdv_client
{
    static mdv_client * connect(mdv_client_config const *config);

    void close();

    mdv_table * createTable(mdv_table_desc *table)
    {
        return mdv_create_table($self, table);
    }

    bool insertRows(mdv_table *table, mdv_rowset *rowset)
    {
        return mdv_insert_rows($self, table, rowset) == MDV_OK;
    }
}
