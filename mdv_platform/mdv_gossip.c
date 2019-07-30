#include "mdv_gossip.h"
#include "mdv_log.h"
#include "mdv_alloc.h"
#include "mdv_algorithm.h"


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

    uint32_t union_size = 0, diff_size = 0;
    uint32_t *union_ids = tmp;
    uint32_t *diff_ids = tmp + src->size + dst->size;

    mdv_union_and_diff_u32(dst->ids, dst->size,
                           src->ids, src->size,
                           union_ids, &union_size,
                           diff_ids, &diff_size);

    if (diff_size)
    {
        // mdv_msg
    }

    mdv_free(tmp, "gossip ids");

    return MDV_OK;
}
