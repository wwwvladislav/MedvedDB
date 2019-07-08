#include "mdv_chaman.h"
#include "mdv_threadpool.h"
#include "mdv_alloc.h"
#include "mdv_log.h"
#include "mdv_rollbacker.h"
#include "mdv_socket.h"
#include "mdv_limits.h"
#include "mdv_timerfd.h"
#include "mdv_hashmap.h"


/// @cond Doxygen_Suppress

struct mdv_chaman;


typedef struct mdv_listener_info
{
    mdv_chaman      *chaman;    ///< chaman
    mdv_descriptor   sock;      ///< server socket
} mdv_listener_info;


typedef struct mdv_peer_info
{
    mdv_chaman      *chaman;    ///< chaman
    mdv_peer_id      uuid;      ///< client uuid
    mdv_descriptor   sock;      ///< client socket
} mdv_peer_info;


struct mdv_chaman
{
    mdv_chaman_config   config;     ///< configuration
    mdv_threadpool     *threadpool; ///< thread pool
    mdv_list            listeners;  ///< listeners list
    mdv_hashmap         peers;      ///< uuid->peer
    mdv_descriptor      timer;      ///< reconnection timer
};

/// @endcond


mdv_chaman * mdv_chaman_create(mdv_chaman_config const *config)
{
    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_chaman *chaman = mdv_alloc(sizeof(mdv_chaman));

    if(!chaman)
    {
        MDV_LOGE("No memory for chaman");
        return 0;
    }

    memset(chaman, 0, sizeof *chaman);

    mdv_rollbacker_push(rollbacker, mdv_free, chaman);

    chaman->config = *config;

    chaman->threadpool = mdv_threadpool_create(config->tp_size);

    if (!chaman->threadpool)
    {
        MDV_LOGE("Threadpool wasn't created for chaman");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_threadpool_free, chaman->threadpool);

    return chaman;
}


void mdv_chaman_free(mdv_chaman *chaman)
{
    if (chaman)
    {
        // Stop thread pool
        mdv_threadpool_free(chaman->threadpool);

        // Close server sockets
        mdv_list_foreach(chaman->listeners, mdv_listener_info, entry)
            mdv_socket_close(entry->data.sock);
        mdv_list_clear(chaman->listeners);

        // Free chaman
        mdv_free(chaman);
    }
}


static void mdv_chaman_in_handler(mdv_descriptor fd, uint32_t events, void *data)
{
    mdv_chaman *chaman = (mdv_chaman *)data;

    static _Thread_local char buffer[1024];

    size_t len = sizeof(buffer) - 1;

    mdv_errno err = mdv_read(fd, buffer, &len);

    while(err == MDV_OK)
    {
        buffer[len] = 0;
        MDV_LOGI("DATA: %s", buffer);

        len = sizeof(buffer) - 1;
        err = mdv_read(fd, buffer, &len);
    }

    if (err == MDV_CLOSED)
    {
        MDV_LOGI("Peer %p was disconnected", fd);
        mdv_threadpool_remove(chaman->threadpool, fd);
    }
    else
    {
        mdv_threadpool_task task =
        {
            .fd = fd,
            .data = chaman,
            .fn = mdv_chaman_in_handler
        };

        err = mdv_threadpool_mod(chaman->threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLIN | MDV_EPOLLERR, &task);

        if (err != MDV_OK)
            MDV_LOGE("Connection modification failed with error '%s' (%u)", mdv_strerror(err), err);
    }
}


static void mdv_chaman_new_peer(mdv_chaman *chaman, mdv_descriptor fd, mdv_sockaddr const *peer)
{
    char buf[MDV_ADDR_LEN_MAX] = { 0 };

    mdv_string str_addr = mdv_str_static(buf);

    mdv_errno err = mdv_sockaddr2str(MDV_SOCK_STREAM, peer, &str_addr);

    if (err == MDV_OK)
        MDV_LOGI("New connection '%s' was accepted", str_addr.ptr);


    mdv_threadpool_task task =
    {
        .fd = fd,
        .data = chaman,
        .fn = mdv_chaman_in_handler
    };

    err = mdv_threadpool_add(chaman->threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLIN | MDV_EPOLLERR, &task);

    if (err != MDV_OK)
    {
        if (mdv_str_empty(str_addr))
            MDV_LOGE("Incoming connection registration failed with error '%s' (%u)", mdv_strerror(err), err);
        else
            MDV_LOGE("Incoming connection '%s' registration failed with error '%s' (%u)", str_addr.ptr, mdv_strerror(err), err);
        mdv_socket_close(fd);
    }
}


static void mdv_chaman_accept_handler(mdv_descriptor fd, uint32_t events, void *data)
{
    mdv_listener_info *listener = (mdv_listener_info *)data;

    mdv_sockaddr peer;

    mdv_descriptor peer_fd = mdv_socket_accept(fd, &peer);

    if (peer_fd != MDV_INVALID_DESCRIPTOR)
        mdv_chaman_new_peer(listener->chaman, peer_fd, &peer);
}


mdv_errno mdv_chaman_listen(mdv_chaman *chaman, mdv_string const addr)
{
    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_socket_type socktype = MDV_SOCK_STREAM;
    mdv_sockaddr sockaddr;

    mdv_errno err = mdv_str2sockaddr(addr, &socktype, &sockaddr);

    if (err)
    {
        MDV_LOGE("Address resolution was failed with error '%s' (%d)", mdv_strerror(err), err);
        return err;
    }


    mdv_descriptor sd = mdv_socket(socktype);

    if(sd == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("Listener socket '%s' was not created", addr.ptr);
        return MDV_FAILED;
    }

    mdv_socket_reuse_addr(sd);
    mdv_socket_nonblock(sd);

    mdv_rollbacker_push(rollbacker, mdv_socket_close, sd);

    err = mdv_socket_bind(sd, &sockaddr);

    if(err != MDV_OK)
    {
        MDV_LOGE("Binding to address '%s' failed with error '%s' (%u)", addr.ptr, mdv_strerror(err), err);
        mdv_rollback(rollbacker);
        return err;
    }


    err = mdv_socket_listen(sd);

    if(err != MDV_OK)
    {
        MDV_LOGE("Address '%s' listening failed with error '%s' (%u)", addr.ptr, mdv_strerror(err), err);
        mdv_rollback(rollbacker);
        return err;
    }


    mdv_listener_info listener =
    {
        .chaman = chaman,
        .sock = sd
    };

    mdv_list_entry_base *entry = mdv_list_push_back(chaman->listeners, listener);

    if (!entry)
    {
        MDV_LOGE("No memory for listener %s", addr.ptr);
        mdv_rollback(rollbacker);
        return MDV_NO_MEM;
    }

    mdv_rollbacker_push(rollbacker, _mdv_list_pop_back, &chaman->listeners);


    mdv_threadpool_task task =
    {
        .fd = sd,
        .data = entry->data,
        .fn = mdv_chaman_accept_handler
    };

    err = mdv_threadpool_add(chaman->threadpool, MDV_EPOLLEXCLUSIVE | MDV_EPOLLIN | MDV_EPOLLERR, &task);

    if (err != MDV_OK)
    {
        MDV_LOGE("Address '%s' listening failed with error '%s' (%u)", addr.ptr, mdv_strerror(err), err);
        mdv_rollback(rollbacker);
        return err;
    }

    return MDV_OK;
}

mdv_errno mdv_chaman_connect(mdv_chaman *chaman, mdv_string const addr)
{
    // TODO: linux/tcp.h
    // SO_SNDLOWAT
    // TCP_DEFER_ACCEPT
    // TCP_KEEPIDLE, TCP_KEEPINTVL, TCP_KEEPCNT
/*
    mdv_socket_type socktype = MDV_SOCK_STREAM;
    mdv_sockaddr sockaddr;

    mdv_errno err = mdv_str2sockaddr(addr, &socktype, &sockaddr);

    if (err)
    {
        MDV_LOGE("Address resolution was failed with error '%s' (%d)", mdv_strerror(err), err);
        return err;
    }


    mdv_descriptor sd = mdv_socket(socktype);

    if(sd == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("Listener socket '%s' was not created", addr.ptr);
        return MDV_FAILED;
    }

    mdv_socket_nonblock(sd);
*/
    return MDV_OK;
}
