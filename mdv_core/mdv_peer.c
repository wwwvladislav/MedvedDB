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


/// Peer context used for storing different type of information about active peer (it should be cast to mdv_conctx)
typedef struct mdv_peer
{
    mdv_cli_type        type;                       ///< Client type
    mdv_tablespace     *tablespace;                 ///< tablespace
    mdv_dispatcher     *dispatcher;                 ///< Messages dispatcher
    mdv_descriptor      sock;                       ///< Socket associated with peer
    mdv_uuid            peer_uuid;                  ///< peer uuid
    mdv_uuid            current_uuid;               ///< current node uuid
    size_t              created_time;               ///< time, when peer registered
    uint8_t             chin:1;                     ///< channel direction (1 - in, 0 - out)
    char                listen[MDV_ADDR_LEN_MAX];   ///< Peer listen address
} mdv_peer;


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

    MDV_LOGI("<<<<< %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(msg->hdr.id));

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_p2p_msg_name(msg->hdr.id));
        return MDV_FAILED;
    }

    mdv_msg_p2p_hello * peer_hello = mdv_unbinn_p2p_hello(&binn_msg);

    if (!peer_hello)
    {
        MDV_LOGE("Invalid '%s' message", mdv_p2p_msg_name(msg->hdr.id));
        binn_free(&binn_msg);
        return MDV_FAILED;
    }

    binn_free(&binn_msg);

    if(peer_hello->version != MDV_VERSION)
    {
        MDV_LOGE("Invalid peer version");
        mdv_free(peer_hello, "msg_p2p_hello");
        return MDV_FAILED;
    }

    peer->peer_uuid = peer_hello->uuid;

    strncpy(peer->listen, peer_hello->listen, sizeof peer->listen);
    peer->listen[sizeof peer->listen - 1] = 0;

    mdv_free(peer_hello, "msg_p2p_hello");

    if(peer->chin)
    {
        mdv_msg_p2p_hello hello =
        {
            .version = MDV_VERSION,
            .uuid = peer->current_uuid,
            .listen = MDV_CONFIG.server.listen.ptr
        };

        return mdv_peer_hello_reply(peer, msg->hdr.number, &hello);
    }

    return MDV_OK;
}


static mdv_errno mdv_peer_hello(mdv_peer *peer)
{
    // Send Tag
    mdv_msg_tag tag =
    {
        .tag = MDV_CLI_PEER
    };

    mdv_errno err = mdv_dispatcher_write_raw(peer->dispatcher, &tag, sizeof tag);

    if (err != MDV_OK)
        return err;

    // Post hello message

    mdv_msg_p2p_hello hello =
    {
        .version = MDV_VERSION,
        .uuid = peer->current_uuid,
        .listen = MDV_CONFIG.server.listen.ptr
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

    err = mdv_peer_post(peer, &message);

    binn_free(&hey);

    return err;
}


mdv_peer * mdv_peer_accept(mdv_tablespace *tablespace, mdv_descriptor fd, mdv_uuid const *current_uuid)
{
    mdv_peer *peer = mdv_alloc(sizeof(mdv_peer), "peer");

    if (!peer)
    {
        MDV_LOGE("No memory to accept peer");
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    MDV_LOGD("Peer %p initialize", peer);

    peer->type          = MDV_CLI_PEER;
    peer->tablespace    = tablespace;
    peer->sock          = fd;
    peer->current_uuid  = *current_uuid;
    peer->created_time  = mdv_gettime();
    peer->chin          = 1;

    mdv_dispatcher_handler const handlers[] =
    {
        { mdv_msg_p2p_hello_id,     &mdv_peer_hello_handler,    peer }
    };

    peer->dispatcher = mdv_dispatcher_create(fd, sizeof handlers / sizeof *handlers, handlers);

    if (!peer->dispatcher)
    {
        MDV_LOGE("Messages dispatcher not created");
        mdv_free(peer, "peer");
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    MDV_LOGD("Peer %p accepted", peer);

    return peer;
}


mdv_peer * mdv_peer_connect(mdv_tablespace *tablespace, mdv_descriptor fd, mdv_uuid const *current_uuid)
{
    mdv_peer *peer = mdv_alloc(sizeof(mdv_peer), "peer");

    if (!peer)
    {
        MDV_LOGE("No memory to accept peer");
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    MDV_LOGD("Peer %p initialize", peer);

    peer->type          = MDV_CLI_PEER;
    peer->tablespace    = tablespace;
    peer->sock          = fd;
    peer->current_uuid  = *current_uuid;
    peer->created_time  = mdv_gettime();
    peer->chin          = 0;

    mdv_dispatcher_handler const handlers[] =
    {
        { mdv_msg_p2p_hello_id,     &mdv_peer_hello_handler,    peer }
    };

    peer->dispatcher = mdv_dispatcher_create(fd, sizeof handlers / sizeof *handlers, handlers);

    if (!peer->dispatcher)
    {
        MDV_LOGE("Messages dispatcher not created");
        mdv_free(peer, "peer");
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    if (mdv_peer_hello(peer) != MDV_OK)
    {
        MDV_LOGD("Peer handshake messahe failed");
        mdv_free(peer, "peer");
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    MDV_LOGD("Peer %p connected", peer);

    return peer;
}


mdv_errno mdv_peer_recv(mdv_peer *peer)
{
    mdv_errno err = mdv_dispatcher_read(peer->dispatcher);

    switch(err)
    {
        case MDV_OK:
        case MDV_EAGAIN:
            break;

        default:
            mdv_socket_shutdown(peer->sock, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
    }

    return err;
}


mdv_errno mdv_peer_send(mdv_peer *peer, mdv_msg *req, mdv_msg *resp, size_t timeout)
{
    MDV_LOGI(">>>>> %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(req->hdr.id));
    return mdv_dispatcher_send(peer->dispatcher, req, resp, timeout);
}


mdv_errno mdv_peer_post(mdv_peer *peer, mdv_msg *msg)
{
    MDV_LOGI(">>>>> %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(msg->hdr.id));
    return mdv_dispatcher_post(peer->dispatcher, msg);
}


static mdv_errno mdv_peer_reply(mdv_peer *peer, mdv_msg const *msg)
{
    MDV_LOGI(">>>>> %s '%s'", mdv_uuid_to_str(&peer->peer_uuid).ptr, mdv_p2p_msg_name(msg->hdr.id));
    return mdv_dispatcher_reply(peer->dispatcher, msg);
}


void mdv_peer_free(mdv_peer *peer)
{
    MDV_LOGD("Peer %p freed", peer);
    mdv_dispatcher_free(peer->dispatcher);
    mdv_free(peer, "peer");
}

