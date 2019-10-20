#include "mdv_node.h"
#include <stdlib.h>
#include <string.h>


size_t mdv_node_size(mdv_node const *node)
{
    return offsetof(mdv_node, addr) + strlen(node->addr) + 1;
}

