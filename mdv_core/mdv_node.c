#include "mdv_node.h"
#include <mdv_limits.h>
#include <mdv_log.h>
#include <stdlib.h>
#include <string.h>


size_t mdv_node_size(mdv_node const *node)
{
    return offsetof(mdv_node, addr) + strlen(node->addr) + 1;
}


bool mdv_node_init(mdv_node *node, mdv_uuid const *uuid, uint32_t id, char const *addr)
{
    size_t const addr_len = strlen(addr);

    if (addr_len >= MDV_ADDR_LEN_MAX)
    {
        MDV_LOGE("Invalid address");
        return false;
    }

    memset(node, 0, sizeof *node);

    node->uuid      = *uuid;
    node->id        = id;

    memcpy(node->addr, addr, addr_len + 1);

    return true;
}
