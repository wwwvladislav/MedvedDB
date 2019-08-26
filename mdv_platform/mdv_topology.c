#include "mdv_topology.h"
#include "mdv_tracker.h"
#include "mdv_vector.h"
#include "mdv_alloc.h"
#include "mdv_log.h"


typedef mdv_vector(mdv_tracker_link) mdv_links_array;


static void mdv_tracker_link_add(mdv_tracker_link const *tracker_link, void *arg)
{
    mdv_links_array *links_array = arg;
    if (!mdv_vector_push_back(*links_array, *tracker_link))
        MDV_LOGE("No memory for network topology link");
}


mdv_topology * mdv_topology_extract(struct mdv_tracker *tracker)
{
    size_t const links_count = mdv_tracker_links_count(tracker);

    mdv_links_array links_array;

    if (!mdv_vector_create(links_array, links_count + 16, mdv_stallocator))
    {
        MDV_LOGE("No memory for network topology");
        return 0;
    }

    mdv_tracker_links_foreach(tracker, &links_array, &mdv_tracker_link_add);

    // TODO

    mdv_vector_free(links_array);

    return 0;
}


void mdv_topology_free(mdv_topology *topology)
{}
