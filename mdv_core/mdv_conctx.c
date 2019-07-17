#include "mdv_conctx.h"
#include <mdv_messages.h>
#include <mdv_log.h>
#include <mdv_chaman.h>
#include "mdv_user.h"
#include "mdv_peer.h"


mdv_errno mdv_conctx_select(mdv_descriptor fd, uint32_t *type)
{
    mdv_msg_tag tag;
    size_t len = sizeof tag;

    mdv_errno err = mdv_read(fd, &tag, &len);

    if (err == MDV_OK)
        *type = tag.tag;

    return err;
}


void * mdv_conctx_create(mdv_tablespace *tablespace, mdv_descriptor fd, mdv_uuid const *uuid, uint32_t type, uint32_t dir)
{
    switch(type)
    {
        case MDV_CLI_USER:
            return mdv_user_accept(tablespace, fd, uuid);

        case MDV_CLI_PEER:
            return dir == MDV_CHIN
                        ? mdv_peer_accept(tablespace, fd, uuid)
                        : mdv_peer_connect(tablespace, fd, uuid);

        default:
            MDV_LOGE("Undefined client type: %u", type);
    }

    return 0;
}


mdv_errno mdv_conctx_recv(void *arg)
{
    mdv_conctx *conctx = arg;

    switch(conctx->type)
    {
        case MDV_CLI_USER:
            return mdv_user_recv(arg);

        case MDV_CLI_PEER:
            return mdv_peer_recv(arg);

        default:
            MDV_LOGE("Undefined client type: %u", conctx->type);
    }

    return MDV_FAILED;
}


void mdv_conctx_close(void *arg)
{
    mdv_conctx *conctx = arg;

    switch(conctx->type)
    {
        case MDV_CLI_USER:
            return mdv_user_free(arg);

        case MDV_CLI_PEER:
            return mdv_peer_free(arg);

        default:
            MDV_LOGE("Undefined client type: %u", conctx->type);
    }
}
