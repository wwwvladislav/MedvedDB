%module mdv

%inline %{
#include <mdv_client.h>
%}

%rename(Client)         mdv_client;
%rename(clientConnect)  mdv_client_connect;
%rename(clientClose)    mdv_client_close;

%nodefault;
typedef struct mdv_client {} mdv_client;
%clearnodefault;

mdv_client * mdv_client_connect(mdv_client_config const *config);
void mdv_client_close(mdv_client *client);
