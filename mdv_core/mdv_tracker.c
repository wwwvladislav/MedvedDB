#include "mdv_tracker.h"
#include <string.h>


void mdv_tracker_create(mdv_tracker *tracker, mdv_nodes *nodes)
{
    tracker->nodes = nodes;
}


void mdv_tracker_free(mdv_tracker *tracker)
{
    memset(tracker, 0, sizeof *tracker);
}


mdv_errno mdv_tracker_reg(mdv_tracker *tracker, mdv_node *node)
{
    return mdv_nodes_reg(tracker->nodes, node);
}


void mdv_tracker_unreg(mdv_tracker *tracker, mdv_uuid const *uuid)
{
    mdv_nodes_unreg(tracker->nodes, uuid);
}
