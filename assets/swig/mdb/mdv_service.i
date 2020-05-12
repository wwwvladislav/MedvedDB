%module mdb

%inline %{
#include <mdv_service.h>
#include <mdv_alloc.h>
%}

%rename(Service) mdv_service;

%nodefault;
typedef struct {} mdv_service;
%clearnodefault;

%extend mdv_service
{
    mdv_service(char const *cfg)
    {
        mdv_alloc_initialize();

        mdv_service *service = mdv_service_create_with_config(cfg);

        if (!service)
        {
            mdv_alloc_finalize();
            return 0;
        }

        return service;
    }

    ~mdv_service()
    {
        mdv_service_free($self);
        mdv_alloc_finalize();
    }

    bool start();

    void stop();
}
