#include "mdv_server.h"
#include "mdv_config.h"
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <mdv_chaman.h>
#include <mdv_rollbacker.h>
#include "mdv_peer.h"


/// @cond Doxygen_Suppress

struct mdv_server
{
    mdv_uuid            uuid;
    mdv_chaman         *chaman;
    mdv_nodes          *nodes;
    mdv_tablespace     *tablespace;
};

/// @endcond


static void * mdv_channel_accept(mdv_descriptor fd, mdv_string const *addr, void *userdata)
{
    mdv_server *server = (mdv_server *)userdata;

    mdv_peer *peer = mdv_peer_accept(server->tablespace, server->nodes, fd, addr, &server->uuid);

    return peer;
}


static mdv_errno mdv_channel_recv(void *channel)
{
    mdv_peer *peer = (mdv_peer *)channel;
    return mdv_peer_recv(peer);
}


static void mdv_channel_close(void *channel)
{
    mdv_peer *peer = (mdv_peer *)channel;
    mdv_peer_free(peer);
}


mdv_server * mdv_server_create(mdv_tablespace *tablespace, mdv_nodes *nodes, mdv_uuid const *uuid)
{
    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_server *server = mdv_alloc(sizeof(mdv_server));

    if(!server)
    {
        MDV_LOGE("Memory allocation for server failed");
        return 0;
    }

    server->uuid = *uuid;
    server->nodes = nodes;
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
            .accept = &mdv_channel_accept,
            .recv = &mdv_channel_recv,
            .close = &mdv_channel_close
        }
    };

    server->chaman = mdv_chaman_create(&config);

    if (!server->chaman)
    {
        MDV_LOGE("Chaman not created");
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

