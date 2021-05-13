%module mdv

%inline %{
#include <mdv_uuid.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
%}

%rename(UUID)         mdv_uuid;

%nodefault;
typedef struct {} mdv_uuid;
%clearnodefault;

%newobject mdv_uuid::toString;

%extend mdv_uuid
{
    mdv_uuid(char const * str)
    {
        mdv_uuid *uuid = malloc(sizeof(mdv_uuid));

        if(!uuid)
        {
            MDV_LOGE("No memory for UUID");
            return 0;
        }

        bool res = mdv_uuid_from_str(uuid, str);

        if(!res)
        {
            MDV_LOGE("Invalid UUID");
            free(uuid);
            return 0;
        }

        return uuid;
    }

    ~mdv_uuid()
    {
        free($self);
    }

    char * toString()
    {
        char *tmp = malloc(MDV_UUID_STR_LEN);

        if(!tmp)
        {
            MDV_LOGE("No memory for UUID string representations");
            return 0;
        }

        mdv_uuid_to_str($self, tmp);

        return tmp;
    }
}
