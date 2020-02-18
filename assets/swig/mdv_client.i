%module mdv

%inline %{
#include <mdv_client.h>
%}


%rename(Client)                 mdv_client;
%rename(clientConnect)          mdv_client_connect;
%rename(clientClose)            mdv_client_close;

%rename(ClientConfig)           mdv_client_config;
%rename(ClientDbConfig)         mdv_client_db_config;
%rename(ClientConnectionConfig) mdv_client_connection_config;
%rename(ClientThreadpoolConfig) mdv_client_threadpool_config;


%nodefault;
typedef struct {} mdv_client;
struct mdv_client_db_config;
struct mdv_client_connection_config;
struct mdv_client_threadpool_config;
%clearnodefault;


%ignore mdv_client_config::mdv_client_config();
%ignore mdv_client_db_config::mdv_client_db_config();

%immutable mdv_client_config::db;
%immutable mdv_client_config::connection;
%immutable mdv_client_config::threadpool;

%immutable mdv_client_db_config::addr;

%extend mdv_client_config
{
    mdv_client_config(char const *addr)
    {
        size_t const addr_len = strlen(addr);

        mdv_client_config *cfg = malloc(sizeof(mdv_client_config) + addr_len + 1);

        if(!cfg)
            return 0;

        char *buff = (char *)(cfg + 1);
        memcpy(buff, addr, addr_len + 1);
        cfg->db.addr = buff;

        return cfg;
    }
}

%include "mdv_client_config.h"


mdv_client * mdv_client_connect(mdv_client_config const *config);
void         mdv_client_close(mdv_client *client);
