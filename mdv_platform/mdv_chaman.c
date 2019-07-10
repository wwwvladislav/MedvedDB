#include "mdv_chaman.h"
#include "mdv_mutex.h"
#include "mdv_alloc.h"
#include "mdv_log.h"
#include "mdv_rollbacker.h"
#include "mdv_socket.h"
#include "mdv_limits.h"
#include "mdv_timerfd.h"
#include "mdv_hashmap.h"
#include <string.h>


/// @cond Doxygen_Suppress

struct mdv_chaman
{
    mdv_chaman_config   config;         ///< configuration
    mdv_threadpool     *threadpool;     ///< thread pool
};


typedef enum
{
    MDV_CT_LISTENER = 0,
    MDV_CT_DIALER,
    MDV_CT_PEER
} mdv_context_type;


typedef struct mdv_context_base
{
    mdv_context_type type;
} mdv_context_base;


typedef struct mdv_listener_context
{
    mdv_context_type type;
    mdv_chaman      *chaman;
} mdv_listener_context;


typedef struct mdv_dialer_context
{
    mdv_context_type type;
    mdv_chaman      *chaman;
    mdv_sockaddr     addr;
    mdv_socket_type  protocol;
} mdv_dialer_context;


typedef struct mdv_peer_context
{
    mdv_context_type type;
    mdv_chaman      *chaman;
    char             dataspace[1];
} mdv_peer_context;


typedef mdv_threadpool_task(mdv_listener_context)   mdv_listener_task;
typedef mdv_threadpool_task(mdv_dialer_context)     mdv_dialer_task;
typedef mdv_threadpool_task(mdv_peer_context)       mdv_peer_task;


static void mdv_chaman_new_peer(mdv_chaman *chaman, mdv_descriptor sock, mdv_sockaddr const *addr);

static void mdv_chaman_dialer_handler(uint32_t events, mdv_threadpool_task_base *task_base);
static void mdv_chaman_recv_handler(uint32_t events, mdv_threadpool_task_base *task_base);
static void mdv_chaman_accept_handler(uint32_t events, mdv_threadpool_task_base *task_base);

/// @endcond


mdv_chaman * mdv_chaman_create(mdv_chaman_config const *config)
{
    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    // Allocate memory
    mdv_chaman *chaman = mdv_alloc(sizeof(mdv_chaman));

    if(!chaman)
    {
        MDV_LOGE("No memory for chaman");
        return 0;
    }

    memset(chaman, 0, sizeof *chaman);

    chaman->config = *config;

    mdv_rollbacker_push(rollbacker, mdv_free, chaman);


    // Create and start thread pool
    chaman->threadpool = mdv_threadpool_create(&config->threadpool);

    if (!chaman->threadpool)
    {
        MDV_LOGE("Threadpool wasn't created for chaman");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_threadpool_free, chaman->threadpool);

    return chaman;
}


static void mdv_fd_close(mdv_descriptor fd, void *data)
{
    mdv_context_base *context = (mdv_context_base *)data;

    switch(context->type)
    {
        case MDV_CT_LISTENER:
        case MDV_CT_PEER:
            mdv_socket_close(fd);
            break;

        default:
            break;
    }
}


void mdv_chaman_free(mdv_chaman *chaman)
{
    if (chaman)
    {
        // Stop thread pool
        mdv_threadpool_stop(chaman->threadpool);

        // Close all file descriptors
        mdv_threadpool_foreach(chaman->threadpool, mdv_fd_close);

        // Free thread pool
        mdv_threadpool_free(chaman->threadpool);

        // Free chaman
        mdv_free(chaman);
    }
}


static void mdv_chaman_dialer_handler(uint32_t events, mdv_threadpool_task_base *task_base)
{
    mdv_dialer_task *task = (mdv_dialer_task*)task_base;
    mdv_descriptor fd = task->fd;
    mdv_chaman *chaman = task->context.chaman;
    mdv_threadpool *threadpool = chaman->threadpool;

    mdv_string str_addr = mdv_sockaddr2str(task->context.protocol, &task->context.addr);

    mdv_threadpool_remove(threadpool, fd);

    if (events & MDV_EPOLLERR)
    {
        if (!mdv_str_empty(str_addr))
            MDV_LOGI("Connection to '%s' failed", str_addr.ptr);
        mdv_socket_close(fd);
    }
    else if (events & MDV_EPOLLOUT)
    {
        if (!mdv_str_empty(str_addr))
            MDV_LOGI("Connection to '%s' established", str_addr.ptr);
        mdv_chaman_new_peer(chaman, fd, &task->context.addr);
    }
}


static void mdv_chaman_recv_handler(uint32_t events, mdv_threadpool_task_base *task_base)
{
    mdv_peer_task *task = (mdv_peer_task*)task_base;
    mdv_descriptor fd = task->fd;
    mdv_chaman *chaman = task->context.chaman;
    mdv_threadpool *threadpool = chaman->threadpool;

    mdv_errno err = chaman->config.channel.recv(chaman->config.userdata, task->context.dataspace, fd);

    if (err == MDV_CLOSED
        || (events & MDV_EPOLLERR))
    {
        MDV_LOGI("Peer %p was disconnected", fd);
        chaman->config.channel.close(chaman->config.userdata, task->context.dataspace);
        mdv_threadpool_remove(threadpool, fd);
        mdv_socket_close(fd);
    }
    else
    {
        err = mdv_threadpool_rearm(threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLIN | MDV_EPOLLERR, (mdv_threadpool_task_base*)task);
        if (err != MDV_OK)
            MDV_LOGE("Connection modification failed with error '%s' (%u)", mdv_strerror(err), err);
    }
}


static void mdv_chaman_new_peer(mdv_chaman *chaman, mdv_descriptor sock, mdv_sockaddr const *addr)
{
    mdv_socket_nonblock(sock);
    mdv_socket_tcp_keepalive(sock, chaman->config.peer.keepidle,
                                   chaman->config.peer.keepcnt,
                                   chaman->config.peer.keepintvl);

    size_t const context_size = offsetof(mdv_peer_context, dataspace)
                                + chaman->config.channel.context.size
                                + chaman->config.channel.context.guardsize;
    size_t const size = offsetof(mdv_threadpool_task(mdv_peer_context), context)
                        + context_size;
    char buff[size];

    mdv_peer_task *task = (mdv_peer_task*)buff;

    task->fd = sock;
    task->fn = mdv_chaman_recv_handler;
    task->context_size = context_size;
    task->context.type = MDV_CT_PEER;
    task->context.chaman = chaman;

    memset(task->context.dataspace, -1, chaman->config.channel.context.size
                                        + chaman->config.channel.context.guardsize);

    mdv_errno err = mdv_threadpool_add(chaman->threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLIN | MDV_EPOLLERR, (mdv_threadpool_task_base const *)task);

    mdv_string str_addr = mdv_sockaddr2str(MDV_SOCK_STREAM, addr);

    if (err != MDV_OK)
    {
        if (mdv_str_empty(str_addr))
            MDV_LOGE("Connection registration failed with error '%s' (%u)", mdv_strerror(err), err);
        else
            MDV_LOGE("Connection with '%s' registration failed with error '%s' (%u)", str_addr.ptr, mdv_strerror(err), err);
        mdv_socket_close(sock);
    }
    else
    {
        if (mdv_str_empty(str_addr))
            MDV_LOGI("New connection was successfully registered");
        else
            MDV_LOGI("New connection with '%s' was successfully registered", str_addr.ptr);
        chaman->config.channel.init(chaman->config.userdata, task->context.dataspace, sock);
    }
}


static void mdv_chaman_accept_handler(uint32_t events, mdv_threadpool_task_base *task_base)
{
    mdv_listener_task *listener_task = (mdv_listener_task*)task_base;

    mdv_sockaddr addr;

    mdv_descriptor sock = mdv_socket_accept(listener_task->fd, &addr);

    mdv_chaman *chaman = listener_task->context.chaman;

    if (sock != MDV_INVALID_DESCRIPTOR)
        mdv_chaman_new_peer(chaman, sock, &addr);
}


mdv_errno mdv_chaman_listen(mdv_chaman *chaman, mdv_string const addr)
{
    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_socket_type socktype = MDV_SOCK_STREAM;
    mdv_sockaddr sockaddr;

    mdv_errno err = mdv_str2sockaddr(addr, &socktype, &sockaddr);

    if (err != MDV_OK)
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
    mdv_socket_tcp_keepalive(sd, chaman->config.peer.keepidle,
                                 chaman->config.peer.keepcnt,
                                 chaman->config.peer.keepintvl);
    mdv_socket_tcp_defer_accept(sd);


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


    mdv_listener_task task =
    {
        .fd = sd,
        .fn = mdv_chaman_accept_handler,
        .context_size = sizeof(mdv_listener_context),
        .context =
        {
            .type = MDV_CT_LISTENER,
            .chaman = chaman
        }
    };

    err = mdv_threadpool_add(chaman->threadpool, MDV_EPOLLEXCLUSIVE | MDV_EPOLLIN | MDV_EPOLLERR, (mdv_threadpool_task_base const *)&task);

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
    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_socket_type socktype = MDV_SOCK_STREAM;
    mdv_sockaddr sockaddr;

    mdv_errno err = mdv_str2sockaddr(addr, &socktype, &sockaddr);

    if (err != MDV_OK)
    {
        MDV_LOGE("Address resolution failed with error '%s' (%d)", mdv_strerror(err), err);
        return err;
    }

    mdv_descriptor sd = mdv_socket(socktype);

    if(sd == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("Listener socket '%s' was not created", addr.ptr);
        return MDV_FAILED;
    }

    mdv_socket_nonblock(sd);

    mdv_rollbacker_push(rollbacker, mdv_socket_close, sd);

    err = mdv_socket_connect(sd, &sockaddr);

    if (err != MDV_INPROGRESS)
    {
        MDV_LOGE("Connection to %s failed with error '%s' (%d)", addr.ptr, mdv_strerror(err), err);
        return err;
    }

    mdv_dialer_task task =
    {
        .fd = sd,
        .fn = mdv_chaman_dialer_handler,
        .context_size = sizeof(mdv_dialer_context),
        .context =
        {
            .type = MDV_CT_DIALER,
            .chaman = chaman,
            .addr = sockaddr,
            .protocol = socktype
        }
    };

    err = mdv_threadpool_add(chaman->threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLOUT | MDV_EPOLLERR, (mdv_threadpool_task_base const *)&task);

    if (err != MDV_OK)
    {
        MDV_LOGE("Address '%s' listening failed with error '%s' (%u)", addr.ptr, mdv_strerror(err), err);
        mdv_rollback(rollbacker);
        return err;
    }

    return MDV_OK;
}
