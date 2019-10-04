#include "mdv_storages.h"
#include <stdio.h>


char const *MDV_STRG_TRLOG(mdv_uuid const *uuid)
{
    static _Thread_local char name[64];
    mdv_string str_uuid = mdv_uuid_to_str(uuid);
    snprintf(name, sizeof name, "%s.mdb", str_uuid.ptr);
    return name;
}


char const *MDV_MAP_TRANSACTION_LOG(uint32_t node_id)
{
    static _Thread_local char name[64];
    snprintf(name, sizeof name, "%u.log", node_id);
    return name;
}
