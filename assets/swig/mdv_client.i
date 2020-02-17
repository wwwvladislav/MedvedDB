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


%include "mdv_client_config.h"


mdv_client * mdv_client_connect(mdv_client_config const *config);
void         mdv_client_close(mdv_client *client);
