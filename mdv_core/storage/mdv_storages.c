#include "mdv_storages.h"
#include <stdio.h>

char const *MDV_MAP_TRANSACTION_LOG(uint32_t node_id)
{
    static _Thread_local char name[64];
    snprintf(name, sizeof name, "%u.log", node_id);
    return name;
}
