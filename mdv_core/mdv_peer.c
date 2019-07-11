#include "mdv_peer.h"
#include <mdv_messages.h>
#include <mdv_version.h>
#include <mdv_timerfd.h>
#include <mdv_log.h>


/**
 * @brief   Say Hey! to peer node
 */
static mdv_errno mdv_peer_wave(mdv_peer *peer, mdv_uuid const *uuid)
{
    mdv_msg_hello const msg =
    {
        .uuid = *uuid,
        .version = MDV_VERSION
    };

    binn hey;

    if (!mdv_binn_hello(&msg, &hey))
        return MDV_FAILED;

    mdv_msg const message =
    {
        .hdr =
        {
            .id = mdv_msg_hello_id,
            .number = atomic_fetch_add_explicit(&peer->id, 1, memory_order_relaxed),
            .size = binn_size(&hey)
        },
        .payload = binn_ptr(&hey)
    };

    mdv_errno err = mdv_peer_send(peer, &message);

    binn_free(&hey);

    return err;
}


/**
 * @brief   Handle incoming HELLO message
 */
static mdv_errno mdv_peer_wave_handler(mdv_peer *peer, binn const *message)
{
    mdv_msg_hello hello = {};

    if (!mdv_unbinn_hello(message, &hello))
    {
        MDV_LOGE("Invalid HEELO message");
        return MDV_FAILED;
    }

    if(hello.version != MDV_VERSION)
    {
        MDV_LOGE("Invalid client version");
        return MDV_FAILED;
    }

    peer->uuid = hello.uuid;
    peer->initialized = 1;

    return MDV_OK;
}


mdv_errno mdv_peer_init(mdv_peer *peer, mdv_descriptor fd, mdv_uuid const *uuid)
{
    peer->fd = fd;
    peer->initialized = 0;
    atomic_init(&peer->id, 0);
    peer->created_time = mdv_gettime();
    memset(&peer->message, 0, sizeof peer->message);
    return mdv_peer_wave(peer, uuid);
}


mdv_errno mdv_peer_recv(mdv_peer *peer)
{
    mdv_errno err = mdv_read_msg(peer->fd, &peer->message);

    if(err == MDV_OK)
    {
        MDV_LOGI("<<<<< from %s '%s'", mdv_uuid_to_str(&peer->uuid).ptr, mdv_msg_name(peer->message.hdr.id));

        // If peer was not initialized the first message might be only hello message.
        if (!peer->initialized && peer->message.hdr.id != mdv_msg_hello_id)
        {
            mdv_free_msg(&peer->message);
            return MDV_FAILED;
        }

        binn binn_msg;

        if(!binn_load(peer->message.payload, &binn_msg))
        {
            MDV_LOGW("Message '%s' discarded", mdv_msg_name(peer->message.hdr.id));
            return MDV_FAILED;
        }

        // Message handlers
        switch(peer->message.hdr.id)
        {
            case mdv_msg_hello_id:
                err = mdv_peer_wave_handler(peer, &binn_msg);
                break;

            default:
                MDV_LOGE("Unknown message received");
                err = MDV_FAILED;
                break;
        }

        binn_free(&binn_msg);

        mdv_free_msg(&peer->message);
    }

    return err;
}


mdv_errno mdv_peer_send(mdv_peer *peer, mdv_msg const *message)
{
    if (peer->initialized)
        MDV_LOGI(">>>>> %s '%s'", mdv_uuid_to_str(&peer->uuid).ptr, mdv_msg_name(message->hdr.id));
    else
        MDV_LOGI(">>>>> '%s'", mdv_msg_name(message->hdr.id));

    return mdv_write_msg(peer->fd, message);
}


void mdv_peer_free(mdv_peer *peer)
{
    mdv_free_msg(&peer->message);
}

