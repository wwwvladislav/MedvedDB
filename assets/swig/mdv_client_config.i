%module mdv

%inline %{
#include <mdv_client_config.h>
%}

%rename(ClientConfig)           mdv_client_config;
%rename(ClientDbConfig)         mdv_client_db_config;
%rename(ClientConnectionConfig) mdv_client_connection_config;
%rename(ClientThreadpoolConfig) mdv_client_threadpool_config;

%ignore mdv_client_connection_config::retry_interval;
%ignore mdv_client_connection_config::response_timeout;

%extend mdv_client_connection_config
{
    uint32_t getRetryInterval()     { return $self->retry_interval; }
    uint32_t getResponseTimeout()   { return $self->response_timeout; }

    void setRetryInterval(uint32_t v)     { $self->retry_interval = v;   }
    void setResponseTimeout(uint32_t v)   { $self->response_timeout = v; }
}

%immutable mdv_client_config::db;
%immutable mdv_client_config::connection;
%immutable mdv_client_config::threadpool;

%immutable mdv_client_db_config::addr;

%ignore mdv_client_config::mdv_client_config();
%ignore mdv_client_db_config::mdv_client_db_config();

%extend mdv_client_config
{
    mdv_client_config(char const *addr)
    {
        size_t const addr_len = strlen(addr);
        size_t const cfg_size = sizeof(mdv_client_config) + addr_len + 1;

        mdv_client_config *cfg = malloc(cfg_size);

        if(!cfg)
            return 0;

        memset(cfg, 0, cfg_size);

        char *buff = (char *)(cfg + 1);
        memcpy(buff, addr, addr_len + 1);
        cfg->db.addr = buff;

        cfg->connection.retry_interval   = 5;
        cfg->connection.timeout          = 15;
        cfg->connection.keepidle         = 5;
        cfg->connection.keepcnt          = 10;
        cfg->connection.keepintvl        = 5;
        cfg->connection.response_timeout = 5;
        cfg->threadpool.size             = 4;

        return cfg;
    }
}

%include "mdv_client_config.h"
