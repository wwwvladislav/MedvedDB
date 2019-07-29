#include "mdv_gossip.h"
#include "mdv_log.h"
#include "mdv_alloc.h"


mdv_errno mdv_gossip_broadcast(mdv_msg const *msg,
                               mdv_gossip_peers const *src,
                               mdv_gossip_peers const *dst,
                               mdv_errno (*post)(void *data, mdv_msg *msg))
{
    uint32_t *tmp = mdv_alloc_tmp((src->size + 2 * dst->size) * sizeof(mdv_gossip_id), "gossip ids");

    if (!tmp)
    {
        MDV_LOGE("No memory for gossip identifiers");
        return MDV_NO_MEM;
    }

    uint32_t union_size = 0, dst_size = 0;
    uint32_t *union_ids = tmp;
    uint32_t *dst_ids = tmp + src->size + dst->size;

/*
    uint32_t i = 0, j = 0;

    while(i < src->size)
    {
        if (src->ids[i] == dst->ids[j])
        {
            ++i;
            ++j;
            continue;
        }

        if(src->ids[i] < dst->ids[j])
        {
            ++i;
            MDV_LOGW("Unknown peer id: %u", src->ids[i]);
            continue;
        }

        // mdv_errno err = post();
    }
*/

    mdv_free(tmp, "gossip ids");

    return MDV_OK;
}
