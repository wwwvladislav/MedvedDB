#include "mdv_server.h"
#include "mdv_config.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <mdv_chaman.h>
#include <mdv_rollbacker.h>
#include <mdv_socket.h>
#include <mdv_timerfd.h>
#include "mdv_peer.h"


/// @cond Doxygen_Suppress

struct mdv_server
{
    mdv_uuid            uuid;
    mdv_chaman         *chaman;
    mdv_tablespace     *tablespace;
};

/// @endcond


static void mdv_channel_init(void *userdata, void *context, mdv_descriptor fd)
{
    mdv_server *server = (mdv_server *)userdata;
    mdv_peer *peer = (mdv_peer *)context;

    peer->fd = fd;
    peer->initialized = 0;
    atomic_init(&peer->req_id, 0);
    peer->created_time = mdv_gettime();

    mdv_errno err = mdv_peer_wave(peer, &server->uuid);

    if (err != MDV_OK)
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
}


static mdv_errno mdv_channel_recv(void *userdata, void *context, mdv_descriptor fd)
{
    (void)context;
    (void)fd;

    return MDV_OK;
}


static void mdv_channel_close(void *userdata, void *context)
{
    (void)context;
}


mdv_server * mdv_server_create(mdv_tablespace *tablespace, mdv_uuid const *uuid)
{
    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_server *server = mdv_alloc(sizeof(mdv_server));

    if(!server)
    {
        MDV_LOGE("Memory allocation for server was failed");
        return 0;
    }

    server->uuid = *uuid;
    server->tablespace = tablespace;

    mdv_rollbacker_push(rollbacker, mdv_free, server);

   mdv_chaman_config const config =
    {
        .peer =
        {
            .keepidle          = MDV_CONFIG.connection.keep_idle,
            .keepcnt           = MDV_CONFIG.connection.keep_count,
            .keepintvl         = MDV_CONFIG.connection.keep_interval
        },
        .threadpool =
        {
            .size = MDV_CONFIG.server.workers,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .userdata = server,
        .channel =
        {
            .context =
            {
                .size = sizeof(mdv_peer),
                .guardsize = 16
            },
            .init = mdv_channel_init,
            .recv = mdv_channel_recv,
            .close = mdv_channel_close
        }
    };

    server->chaman = mdv_chaman_create(&config);

    if (!server->chaman)
    {
        MDV_LOGE("Chaman was not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_chaman_free, server->chaman);

    mdv_errno err = mdv_chaman_listen(server->chaman, MDV_CONFIG.server.listen);

    if (err != MDV_OK)
    {
        MDV_LOGE("Listener failed with error: %s (%d)", mdv_strerror(err), err);
        mdv_rollback(rollbacker);
        return 0;
    }

    return server;
}


bool mdv_server_start(mdv_server *srvr)
{
    return true;
}


void mdv_server_free(mdv_server *srvr)
{
    if (srvr)
    {
        mdv_chaman_free(srvr->chaman);
        mdv_free(srvr);
    }
}


bool mdv_server_handler_reg(mdv_server *srvr, uint32_t msg_id, mdv_message_handler handler)
{
/*
    if (msg_id >= sizeof srvr->handlers / sizeof *srvr->handlers)
    {
        MDV_LOGE("Server message handler not registered. Message ID %u is invalid.", msg_id);
        return false;
    }

    srvr->handlers[msg_id] = handler;
*/
    return true;
}

