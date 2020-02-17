#include "mdv_storages.h"
#include <stdio.h>


char const *MDV_STRG_UUID(mdv_uuid const *uuid)
{
    static _Thread_local char name[64];
    snprintf(name, sizeof name, "%s.mdb", mdv_uuid_to_str(uuid));
    return name;
}
