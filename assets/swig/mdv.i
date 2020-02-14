%module mdv

%inline %{
#include <mdv_client.h>
%}

%include "stdint.i"

/*
typedef struct mdv_client mdv_client;

typedef struct
{
    struct
    {
        char const *addr;               ///< DB address
    } db;                               ///< DB settings

    struct
    {
        uint32_t    retry_interval;     ///< Interval between reconnections (in seconds)
        uint32_t    timeout;            ///< Connection timeout (in seconds)
        uint32_t    keepidle;           ///< Start keeplives after this period (in seconds)
        uint32_t    keepcnt;            ///< Number of keepalives before death
        uint32_t    keepintvl;          ///< Interval between keepalives (in seconds)
        uint32_t    response_timeout;   ///< Timeout for responses (in seconds)
    } connection;                       ///< connection configuration

    struct
    {
        size_t      size;               ///< threads count in thread pool
    } threadpool;                       ///< thread pool options
} mdv_client_config;

mdv_client * mdv_client_connect(mdv_client_config const *config);
*/
