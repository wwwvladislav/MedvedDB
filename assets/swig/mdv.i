%module mdv

%inline %{
#include <mdv_client.h>
%}

%include "stdint.i"

%rename(ClientConfig) mdv_client_config;
%rename(ClientConfigDb) mdv_client_config_db;
%rename(ClientConfigConnection) mdv_client_config_connection;
%rename(ClientConfigThreadpool) mdv_client_config_threadpool;

%include "mdv_client_config.h"
%include "mdv_client.i"
