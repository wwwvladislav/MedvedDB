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


mdv_conctx * mdv_conctx_create(mdv_conctx_config const *config, uint32_t type, uint32_t dir)
{
    switch(type)
    {
        case MDV_CLI_USER:
            return (mdv_conctx*)mdv_user_accept(config);

        case MDV_CLI_PEER:
            return dir == MDV_CHIN
                        ? (mdv_conctx*)mdv_peer_accept(config)
                        : (mdv_conctx*)mdv_peer_connect(config);

        default:
            MDV_LOGE("Undefined client type: %u", type);
    }

    return 0;
}


mdv_errno mdv_conctx_recv(mdv_conctx *conctx)
{
    switch(conctx->type)
    {
        case MDV_CLI_USER:
            return mdv_user_recv((mdv_user *)conctx);

        case MDV_CLI_PEER:
            return mdv_peer_recv((mdv_peer *)conctx);

        default:
            MDV_LOGE("Undefined client type: %u", conctx->type);
    }

    return MDV_FAILED;
}


void mdv_conctx_free(mdv_conctx *conctx)
{
    switch(conctx->type)
    {
        case MDV_CLI_USER:
            return mdv_user_free((mdv_user *)conctx);

        case MDV_CLI_PEER:
            return mdv_peer_free((mdv_peer *)conctx);

        default:
            MDV_LOGE("Undefined client type: %u", conctx->type);
    }
}
