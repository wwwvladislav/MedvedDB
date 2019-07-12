#include "mdv_peer.h"
#include <mdv_messages.h>
#include <mdv_version.h>
#include <mdv_timerfd.h>
#include <mdv_log.h>
#include <stddef.h>
#include <string.h>


static mdv_errno mdv_peer_wave(mdv_peer *peer, uint16_t id, mdv_msg_hello const *msg)
{
    binn hey;

    if (!mdv_binn_hello(msg, &hey))
        return MDV_FAILED;

    mdv_msg const message =
    {
        .hdr =
        {
            .id = mdv_msg_hello_id,
            .number = id,
            .size = binn_size(&hey)
        },
        .payload = binn_ptr(&hey)
    };

    mdv_errno err = mdv_peer_send(peer, &message);

    binn_free(&hey);

    return err;
}


static mdv_errno mdv_peer_status(mdv_peer *peer, uint16_t id, mdv_msg_status const *msg)
{
    binn status;

    if (!mdv_binn_status(msg, &status))
        return MDV_FAILED;

    mdv_msg const message =
    {
        .hdr =
        {
            .id = mdv_msg_status_id,
            .number = id,
            .size = binn_size(&status)
        },
        .payload = binn_ptr(&status)
    };

    mdv_errno err = mdv_peer_send(peer, &message);

    binn_free(&status);

    return err;
}


static mdv_errno mdv_peer_table_info(mdv_peer *peer, uint16_t id, mdv_msg_table_info const *msg)
{
    binn table_info;

    if (!mdv_binn_table_info(msg, &table_info))
        return MDV_FAILED;

    mdv_msg const message =
    {
        .hdr =
        {
            .id = mdv_msg_table_info_id,
            .number = id,
            .size = binn_size(&table_info)
        },
        .payload = binn_ptr(&table_info)
    };

    mdv_errno err = mdv_peer_send(peer, &message);

    binn_free(&table_info);

    return err;
}


static mdv_errno mdv_peer_wave_handler(mdv_peer *peer, binn const *message)
{
    mdv_msg_hello hello = {};

    if (!mdv_unbinn_hello(message, &hello))
    {
        MDV_LOGE("Invalid '%s' message", mdv_msg_name(mdv_msg_hello_id));
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


static mdv_errno mdv_peer_create_table_handler(mdv_peer *peer, uint16_t id, binn const *message)
{
    mdv_msg_create_table_base *create_table = mdv_unbinn_create_table(message);

    if (!create_table)
    {
        MDV_LOGE("Invalid '%s' message", mdv_msg_name(mdv_msg_create_table_id));
        return MDV_FAILED;
    }

    mdv_errno err = mdv_tablespace_create_table(peer->tablespace, (mdv_table_base*)&create_table->table);

    if (err == MDV_OK)
    {
        mdv_msg_table_info const msg =
        {
            .uuid = create_table->table.uuid
        };

        err = mdv_peer_table_info(peer, id, &msg);
    }
    else
    {
        mdv_msg_status const msg =
        {
            .err = err,
            .message = { 0 }
        };

        err = mdv_peer_status(peer, id, &msg);
    }

    return err;
}


mdv_errno mdv_peer_init(mdv_peer *peer, mdv_tablespace *tablespace, mdv_descriptor fd, mdv_uuid const *uuid)
{
    peer->tablespace = tablespace;
    peer->fd = fd;
    peer->initialized = 0;
    atomic_init(&peer->id, 0);
    peer->created_time = mdv_gettime();
    memset(&peer->message, 0, sizeof peer->message);

    mdv_msg_hello const msg =
    {
        .uuid = *uuid,
        .version = MDV_VERSION
    };

    return mdv_peer_wave(peer, 0, &msg);
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

            case mdv_msg_create_table_id:
                err = mdv_peer_create_table_handler(peer, peer->message.hdr.number, &binn_msg);
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

