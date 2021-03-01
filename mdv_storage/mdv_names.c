#include "mdv_names.h"
#include <stdio.h>


char const * MDV_STRG_UUID(mdv_uuid const *uuid, char *name, size_t size)
{
    char uuid_str[MDV_UUID_STR_LEN];
    snprintf(name, size, "%s.mdb", mdv_uuid_to_str(uuid, uuid_str));
    return name;
}
