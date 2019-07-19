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


mdv_errno mdv_tracker_register(mdv_tracker *tracker, mdv_node *node)
{
    return MDV_OK;
}
