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
    MDV_CT_SELECTOR = 0,
    MDV_CT_LISTENER,
    MDV_CT_DIALER,
    MDV_CT_PEER
} mdv_context_type;


typedef struct mdv_context_base
{
    mdv_context_type type;
} mdv_context_base;


typedef struct mdv_selector_context
{
    mdv_context_type type;
    mdv_chaman      *chaman;
    mdv_sockaddr     addr;
} mdv_selector_context;


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
    uint8_t          channel_type;
} mdv_dialer_context;


typedef struct mdv_peer_context
{
    mdv_context_type type;
    mdv_chaman      *chaman;
    void            *peer;
} mdv_peer_context;


typedef mdv_threadpool_task(mdv_listener_context)   mdv_listener_task;
typedef mdv_threadpool_task(mdv_selector_context)   mdv_selector_task;
typedef mdv_threadpool_task(mdv_dialer_context)     mdv_dialer_task;
typedef mdv_threadpool_task(mdv_peer_context)       mdv_peer_task;


static void mdv_chaman_new_connection(mdv_chaman *chaman, mdv_descriptor sock, mdv_sockaddr const *addr, uint8_t type, mdv_channel_dir dir);

static void mdv_chaman_dialer_handler(uint32_t events, mdv_threadpool_task_base *task_base);
static void mdv_chaman_recv_handler(uint32_t events, mdv_threadpool_task_base *task_base);
static void mdv_chaman_accept_handler(uint32_t events, mdv_threadpool_task_base *task_base);
static void mdv_chaman_select_handler(uint32_t events, mdv_threadpool_task_base *task_base);


/// @endcond


mdv_chaman * mdv_chaman_create(mdv_chaman_config const *config)
{
    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    // Allocate memory
    mdv_chaman *chaman = mdv_alloc(sizeof(mdv_chaman), "chaman");

    if(!chaman)
    {
        MDV_LOGE("No memory for chaman");
        return 0;
    }

    memset(chaman, 0, sizeof *chaman);

    chaman->config = *config;

    mdv_rollbacker_push(rollbacker, mdv_free, chaman, "chaman");


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
        case MDV_CT_SELECTOR:
        case MDV_CT_LISTENER:
            mdv_socket_close(fd);
            break;

        case MDV_CT_PEER:
        {
            mdv_peer_context *peer_context = data;
            peer_context->chaman->config.channel.close(peer_context->peer);
            mdv_socket_close(fd);
            break;
        }

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
        mdv_free(chaman, "chaman");
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
        mdv_chaman_new_connection(chaman, fd, &task->context.addr, task->context.channel_type, MDV_CHOUT);
    }
}


static void mdv_chaman_recv_handler(uint32_t events, mdv_threadpool_task_base *task_base)
{
    mdv_peer_task *task = (mdv_peer_task*)task_base;
    mdv_descriptor fd = task->fd;
    mdv_chaman *chaman = task->context.chaman;
    mdv_threadpool *threadpool = chaman->threadpool;

    mdv_errno err = chaman->config.channel.recv(task->context.peer);

    if ((events & MDV_EPOLLERR) == 0 && (err == MDV_EAGAIN || err == MDV_OK))
    {
        err = mdv_threadpool_rearm(threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLIN | MDV_EPOLLERR, (mdv_threadpool_task_base*)task);
        if (err != MDV_OK)
            MDV_LOGE("Connection modification failed with error '%s' (%u)", mdv_strerror(err), err);
    }
    else
    {
        MDV_LOGI("Peer %p disconnected", fd);
        chaman->config.channel.close(task->context.peer);
        mdv_threadpool_remove(threadpool, fd);
        mdv_socket_close(fd);
    }
}


static void mdv_chaman_new_connection(mdv_chaman *chaman, mdv_descriptor sock, mdv_sockaddr const *addr, uint8_t type, mdv_channel_dir dir)
{
    mdv_socket_nonblock(sock);
    mdv_socket_tcp_keepalive(sock, chaman->config.channel.keepidle,
                                   chaman->config.channel.keepcnt,
                                   chaman->config.channel.keepintvl);

    mdv_string str_addr = mdv_sockaddr2str(MDV_SOCK_STREAM, addr);

    mdv_peer_task task =
    {
        .fd = sock,
        .fn = mdv_chaman_recv_handler,
        .context_size = sizeof(mdv_peer_context),
        .context =
        {
            .type = MDV_CT_PEER,
            .chaman = chaman,
            .peer = chaman->config.channel.create(sock, &str_addr, chaman->config.userdata, type, dir)
        }
    };

    if (!task.context.peer
        || !mdv_threadpool_add(chaman->threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLIN | MDV_EPOLLERR, (mdv_threadpool_task_base const *)&task))
    {
        if (mdv_str_empty(str_addr))
            MDV_LOGE("Connection registration failed");
        else
            MDV_LOGE("Connection with '%s' registration failed", str_addr.ptr);
        if (task.context.peer)
            chaman->config.channel.close(task.context.peer);
        mdv_socket_close(sock);
    }
    else
    {
        if (mdv_str_empty(str_addr))
            MDV_LOGI("New connection successfully registered");
        else
            MDV_LOGI("New connection with '%s' successfully registered", str_addr.ptr);
    }
}



static void mdv_chaman_select_handler(uint32_t events, mdv_threadpool_task_base *task_base)
{
    mdv_selector_task *selector_task = (mdv_selector_task*)task_base;
    mdv_chaman *chaman = selector_task->context.chaman;

    uint8_t type = 0;

    mdv_errno err = chaman->config.channel.select(selector_task->fd, &type);

    if ((events & MDV_EPOLLERR) == 0 && err == MDV_EAGAIN)
    {
        err = mdv_threadpool_rearm(chaman->threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLIN | MDV_EPOLLERR, (mdv_threadpool_task_base*)selector_task);

        if (err != MDV_OK)
        {
            MDV_LOGE("Connection modification failed with error '%s' (%u)", mdv_strerror(err), err);
            mdv_threadpool_remove(chaman->threadpool, selector_task->fd);
            mdv_socket_close(selector_task->fd);
        }
    }
    else
    {
        mdv_threadpool_remove(chaman->threadpool, selector_task->fd);

        if (err == MDV_OK)
        {
            mdv_chaman_new_connection(chaman, selector_task->fd, &selector_task->context.addr, type, MDV_CHIN);
        }
        else
        {
            mdv_string str_addr = mdv_sockaddr2str(MDV_SOCK_STREAM, &selector_task->context.addr);
            MDV_LOGE("Channel selection failed for %s", str_addr.ptr);
            mdv_socket_close(selector_task->fd);
        }
    }
}


static void mdv_chaman_accept_handler(uint32_t events, mdv_threadpool_task_base *task_base)
{
    mdv_listener_task *listener_task = (mdv_listener_task*)task_base;

    mdv_sockaddr addr;

    mdv_descriptor sock = mdv_socket_accept(listener_task->fd, &addr);

    mdv_chaman *chaman = listener_task->context.chaman;

    if (sock != MDV_INVALID_DESCRIPTOR)
    {
        mdv_socket_nonblock(sock);
        mdv_socket_tcp_keepalive(sock, chaman->config.channel.keepidle,
                                       chaman->config.channel.keepcnt,
                                       chaman->config.channel.keepintvl);

        mdv_selector_task selector_task =
        {
            .fd = sock,
            .fn = mdv_chaman_select_handler,
            .context_size = sizeof(mdv_selector_task),
            .context =
            {
                .type = MDV_CT_SELECTOR,
                .chaman = chaman,
                .addr = addr
            }
        };

        if (!mdv_threadpool_add(chaman->threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLIN | MDV_EPOLLERR, (mdv_threadpool_task_base const *)&selector_task))
        {
            mdv_string str_addr = mdv_sockaddr2str(MDV_SOCK_STREAM, &addr);
            MDV_LOGE("Connection '%s' accepting failed", str_addr.ptr);
            mdv_socket_close(sock);
        }
    }
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
        MDV_LOGE("Address resolution failed with error '%s' (%d)", mdv_strerror(err), err);
        return err;
    }


    mdv_descriptor sd = mdv_socket(socktype);

    if(sd == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("Listener socket '%s' not created", addr.ptr);
        return MDV_FAILED;
    }

    mdv_socket_reuse_addr(sd);
    mdv_socket_nonblock(sd);
    mdv_socket_tcp_keepalive(sd, chaman->config.channel.keepidle,
                                 chaman->config.channel.keepcnt,
                                 chaman->config.channel.keepintvl);
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

    if (!mdv_threadpool_add(chaman->threadpool, MDV_EPOLLEXCLUSIVE | MDV_EPOLLIN | MDV_EPOLLERR, (mdv_threadpool_task_base const *)&task))
    {
        MDV_LOGE("Address '%s' listening failed", addr.ptr);
        mdv_rollback(rollbacker);
        return err;
    }

    return MDV_OK;
}


mdv_errno mdv_chaman_connect(mdv_chaman *chaman, mdv_string const addr, uint8_t type)
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
        MDV_LOGE("Listener socket '%s' not created", addr.ptr);
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
            .type         = MDV_CT_DIALER,
            .chaman       = chaman,
            .addr         = sockaddr,
            .protocol     = socktype,
            .channel_type = type
        }
    };

    if (!mdv_threadpool_add(chaman->threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLOUT | MDV_EPOLLERR, (mdv_threadpool_task_base const *)&task))
    {
        MDV_LOGE("Connection to %s failed", addr.ptr);
        mdv_rollback(rollbacker);
        return err;
    }

    return MDV_OK;
}
