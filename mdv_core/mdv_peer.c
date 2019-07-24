#include "mdv_peer.h"
#include "mdv_p2pmsg.h"
#include "mdv_config.h"
#include <mdv_version.h>
#include <mdv_timerfd.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_socket.h>
#include <mdv_time.h>
#include <mdv_limits.h>
#include <stddef.h>
#include <string.h>


static mdv_errno mdv_peer_reply(mdv_peer *peer, mdv_msg const *msg);


static mdv_errno mdv_peer_hello_reply(mdv_peer *peer, uint16_t id, mdv_msg_p2p_hello const *msg)
{
    binn hey;

    if (!mdv_binn_p2p_hello(msg, &hey))
        return MDV_FAILED;

    mdv_msg message =
    {
        .hdr =
        {
            .id     = mdv_message_id(p2p_hello),
            .number = id,
            .size   = binn_size(&hey)
        },
        .payload = binn_ptr(&hey)
    };

    mdv_errno err = mdv_peer_reply(peer, &message);

    binn_free(&hey);

    return err;
}


static mdv_errno mdv_peer_hello_handler(mdv_msg const *msg, void *arg)
{
    mdv_peer *peer = arg;

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_p2p_msg_name(msg->hdr.id));
        return MDV_FAILED;
    }

    mdv_msg_p2p_hello *peer_hello = mdv_unbinn_p2p_hello(&binn_msg);

    if (!peer_hello)
    {
        MDV_LOGE("Invalid '%s' message", mdv_p2p_msg_name(msg->hdr.id));
        binn_free(&binn_msg);
        return MDV_FAILED;
    }

    binn_free(&binn_msg);

    MDV_LOGI("<<<<< %s '%s'", mdv_uuid_to_str(&peer_hello->uuid).ptr, mdv_p2p_msg_name(msg->hdr.id));

    if(peer_hello->version != MDV_VERSION)
    {
        MDV_LOGE("Invalid peer version");
        mdv_free(peer_hello, "msg_p2p_hello");
        return MDV_FAILED;
    }

    peer->peer_uuid = peer_hello->uuid;

    mdv_errno err = peer->conctx->reg_peer(peer->conctx, peer_hello->listen, &peer->peer_uuid, &peer->peer_id);

    mdv_free(peer_hello, "msg_p2p_hello");

    if (err != MDV_OK)
    {
        MDV_LOGE("Peer registration failed with error '%s' (%d)", mdv_strerror(err), err);
        return err;
    }


    if(peer->conctx->dir == MDV_CHIN)
    {
        mdv_msg_p2p_hello hello =
        {
            .version = MDV_VERSION,
            .uuid    = peer->conctx->cluster->uuid,
            .listen  = MDV_CONFIG.server.listen.ptr
        };

        return mdv_peer_hello_reply(peer, msg->hdr.number, &hello);
    }

    return MDV_OK;
}


static mdv_errno mdv_peer_hello(mdv_peer *peer)
{
    // Post hello message

    mdv_msg_p2p_hello hello =
    {
        .version = MDV_VERSION,
        .uuid    = peer->conctx->cluster->uuid,
        .listen  = MDV_CONFIG.server.listen.ptr
    };

    binn hey;

    if (!mdv_binn_p2p_hello(&hello, &hey))
        return MDV_FAILED;

    mdv_msg message =
    {
        .hdr =
        {
            .id = mdv_message_id(p2p_hello),
            .size = binn_size(&hey)
        },
        .payload = binn_ptr(&hey)
    };

    mdv_errno err = mdv_peer_post(peer, &message);

    binn_free(&hey);

    return err;
}


mdv_errno mdv_peer_init(void *ctx, mdv_conctx *conctx, void *userdata)
{
    mdv_peer *peer = ctx;

    MDV_LOGD("Peer %p initialize", peer);

    peer->tablespace    = userdata;
    peer->conctx        = conctx;
    peer->peer_id       = 0;

    mdv_dispatcher_handler const handlers[] =
    {
        { mdv_msg_p2p_hello_id,     &mdv_peer_hello_handler,    peer }
    };

    for(size_t i = 0; i < sizeof handlers / sizeof *handlers; ++i)
    {
        if (mdv_dispatcher_reg(conctx->dispatcher, handlers + i) != MDV_OK)
        {
            MDV_LOGE("Messages dispatcher handler not registered");
            return MDV_FAILED;
        }
    }

    if (conctx->dir == MDV_CHOUT)
    {
        if (mdv_peer_hello(peer) != MDV_OK)
        {
            MDV_LOGD("Peer handshake message failed");
            return MDV_FAILED;
        }
    }

    MDV_LOGD("Peer %p initialized", peer);

    return MDV_OK;
}


mdv_errno mdv_peer_send(mdv_peer *peer, mdv_msg *req, mdv_msg *resp, size_t timeout)
{
    MDV_LOGI(">>>>> %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(req->hdr.id));
    return mdv_dispatcher_send(peer->conctx->dispatcher, req, resp, timeout);
}


mdv_errno mdv_peer_post(mdv_peer *peer, mdv_msg *msg)
{
    MDV_LOGI(">>>>> %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(msg->hdr.id));
    return mdv_dispatcher_post(peer->conctx->dispatcher, msg);
}


static mdv_errno mdv_peer_reply(mdv_peer *peer, mdv_msg const *msg)
{
    MDV_LOGI(">>>>> %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(msg->hdr.id));
    return mdv_dispatcher_reply(peer->conctx->dispatcher, msg);
}


void mdv_peer_free(void *ctx, mdv_conctx *conctx)
{
    mdv_peer *peer = ctx;
    MDV_LOGD("Peer %p freed", peer);
    conctx->unreg_peer(conctx, &peer->peer_uuid);
}
