#include "mdv_chaman.h"
#include "mdv_socket.h"
#include <mdv_mutex.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <mdv_limits.h>
#include <mdv_timerfd.h>
#include <mdv_time.h>
#include <mdv_hashmap.h>
#include <mdv_hash.h>
#include <string.h>
#include <stdatomic.h>
#include <assert.h>


/// @cond Doxygen_Suppress

struct mdv_chaman
{
    mdv_chaman_config   config;         ///< configuration
    mdv_threadpool     *threadpool;     ///< thread pool
    mdv_mutex           mutex;          ///< Mutex for dialers and channels guard
    mdv_hashmap        *dialers;        ///< Dialers (hashmap<mdv_dialer>)
    mdv_hashmap        *channels;       ///< Channels (hashmap<mdv_channel_ref>)
};


typedef enum
{
    MDV_DIALER_DISCONNECTED,
    MDV_DIALER_CONNECTED,
    MDV_DIALER_CONNECTING,
} mdv_dialer_state;


typedef struct
{
    mdv_netaddr         addr;           ///< Peer address
    mdv_dialer_state    state;          ///< Dialer state
    size_t              attempt_time;   ///< Connection attempt time
    mdv_uuid            channel_id;     ///< Channel identifier (valid only for MDV_DIALER_CONNECTED state)
    mdv_channel_t       channel_type;   ///< Channel type
} mdv_dialer;


typedef struct
{
    mdv_uuid        id;                 ///< Channel identifier
    mdv_channel    *channel;            ///< Channel
    mdv_dialer     *dialer;             ///< Dialer
} mdv_channel_ref;


typedef enum
{
    MDV_CT_SELECTOR = 0,
    MDV_CT_LISTENER,
    MDV_CT_DIALER,
    MDV_CT_RECV,
    MDV_CT_TIMER,
} mdv_context_type;


typedef struct mdv_context_base
{
    mdv_context_type type;
} mdv_context_base;


typedef struct mdv_selector_context
{
    mdv_context_type type;
    mdv_chaman      *chaman;
    mdv_netaddr      addr;
    mdv_channel_dir  dir;
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
    mdv_netaddr      addr;
    mdv_channel_t    channel_type;
} mdv_dialer_context;


typedef struct mdv_recv_context
{
    mdv_context_type type;
    mdv_chaman      *chaman;
    mdv_netaddr     addr;
    mdv_channel     *channel;
} mdv_recv_context;


typedef struct mdv_timer_context
{
    mdv_context_type type;
    mdv_chaman      *chaman;
} mdv_timer_context;


typedef mdv_threadpool_task(mdv_listener_context)   mdv_listener_task;
typedef mdv_threadpool_task(mdv_selector_context)   mdv_selector_task;
typedef mdv_threadpool_task(mdv_dialer_context)     mdv_dialer_task;
typedef mdv_threadpool_task(mdv_recv_context)       mdv_recv_task;
typedef mdv_threadpool_task(mdv_timer_context)      mdv_timer_task;


static void mdv_chaman_new_connection(mdv_chaman         *chaman,
                                      mdv_descriptor      sock,
                                      mdv_netaddr const  *addr,
                                      mdv_channel_t       channel_type,
                                      mdv_channel_dir     dir,
                                      mdv_uuid const *channel_id);
static mdv_errno mdv_chaman_connect(mdv_chaman         *chaman,
                                    mdv_netaddr const  *netaddr,
                                    mdv_channel_t       channel_type);

static void mdv_chaman_dialer_handler(uint32_t events, mdv_threadpool_task_base *task_base);
static void mdv_chaman_recv_handler(uint32_t events, mdv_threadpool_task_base *task_base);
static void mdv_chaman_accept_handler(uint32_t events, mdv_threadpool_task_base *task_base);
static void mdv_chaman_select_handler(uint32_t events, mdv_threadpool_task_base *task_base);
static void mdv_chaman_timer_handler(uint32_t events, mdv_threadpool_task_base *task_base);

static mdv_errno mdv_chaman_dialer_reg(mdv_chaman         *chaman,
                                       mdv_netaddr const * netaddr,
                                       mdv_channel_t       channel_type,
                                       mdv_dialer        **dialer);
static void mdv_chaman_dialer_unreg(mdv_chaman *chaman,
                                    mdv_netaddr const *addr);
static void mdv_chaman_dialer_state(mdv_chaman        *chaman,
                                    mdv_netaddr const *addr,
                                    mdv_dialer_state   state);

static mdv_errno mdv_chaman_selector_reg(mdv_chaman         *chaman,
                                         mdv_descriptor      fd,
                                         mdv_netaddr const  *addr,
                                         mdv_channel_dir     dir);
/// @endcond


static size_t mdv_netaddr_hash(mdv_netaddr const *addr)
{
    return mdv_hash_murmur2a(&addr->sock.addr, sizeof addr->sock.addr, 0);
}


static int mdv_netaddr_cmp(mdv_netaddr const *a, mdv_netaddr const *b)
{
    return memcmp(&a->sock.addr, &b->sock.addr, sizeof a->sock.addr);
}


mdv_chaman * mdv_chaman_create(mdv_chaman_config const *config)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(6);

    // Allocate memory
    mdv_chaman *chaman = mdv_alloc(sizeof(mdv_chaman), "chaman");

    if(!chaman)
    {
        MDV_LOGE("No memory for chaman");
        mdv_rollback(rollbacker);
        return 0;
    }

    memset(chaman, 0, sizeof *chaman);

    chaman->config = *config;

    mdv_rollbacker_push(rollbacker, mdv_free, chaman, "chaman");

    // Create mutex for dialers guard
    if (mdv_mutex_create(&chaman->mutex) != MDV_OK)
    {
        MDV_LOGE("Mutex creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &chaman->mutex);

    // Create dialers map
    chaman->dialers = mdv_hashmap_create(mdv_dialer,
                                         addr,
                                         4,
                                         mdv_netaddr_hash,
                                         mdv_netaddr_cmp);
    if (!chaman->dialers)
    {
        MDV_LOGE("There is no memory for dialers map");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, chaman->dialers);

    // Create channels map
    chaman->channels = mdv_hashmap_create(mdv_channel_ref,
                                         id,
                                         4,
                                         mdv_uuid_hash,
                                         mdv_uuid_cmp);
    if (!chaman->channels)
    {
        MDV_LOGE("There is no memory for channels map");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, chaman->channels);

    // Create and start thread pool
    chaman->threadpool = mdv_threadpool_create(&config->threadpool);

    if (!chaman->threadpool)
    {
        MDV_LOGE("Threadpool wasn't created for chaman");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_threadpool_free, chaman->threadpool);

    mdv_descriptor timer = mdv_timerfd();

    if(timer == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("Timer creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_timerfd_close, timer);

    if (mdv_timerfd_settime(timer, 1 * 1000, config->channel.retry_interval * 1000) != MDV_OK)
    {
        MDV_LOGE("Timer creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_timer_task task =
    {
        .fd = timer,
        .fn = mdv_chaman_timer_handler,
        .context_size = sizeof(mdv_timer_context),
        .context =
        {
            .type         = MDV_CT_DIALER,
            .chaman       = chaman
        }
    };

    if (!mdv_threadpool_add(chaman->threadpool, MDV_EPOLLEXCLUSIVE | MDV_EPOLLIN, (mdv_threadpool_task_base const *)&task))
    {
        MDV_LOGE("Timer registration failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_free(rollbacker);

    return chaman;
}


static void mdv_fd_close(mdv_descriptor fd, void *data)
{
    mdv_context_base *context = (mdv_context_base *)data;

    switch(context->type)
    {
        case MDV_CT_SELECTOR:
        case MDV_CT_LISTENER:
        case MDV_CT_DIALER:
            mdv_socket_close(fd);
            break;

        case MDV_CT_RECV:
            mdv_socket_close(fd);
            break;

        case MDV_CT_TIMER:
            mdv_timerfd_close(fd);
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

        // Free mutex
        mdv_mutex_free(&chaman->mutex);

        // Free dialers map
        mdv_hashmap_release(chaman->dialers);

        // Free channels map
        mdv_hashmap_foreach(chaman->channels, mdv_channel_ref, entry)
            mdv_channel_release(entry->channel);
        mdv_hashmap_release(chaman->channels);

        // Free chaman
        mdv_free(chaman, "chaman");
    }
}


static void mdv_chaman_dialer_handler(uint32_t events, mdv_threadpool_task_base *task_base)
{
    mdv_dialer_task    *task            = (mdv_dialer_task*)task_base;
    mdv_descriptor      fd              = task->fd;
    mdv_chaman         *chaman          = task->context.chaman;
    mdv_threadpool     *threadpool      = chaman->threadpool;
    mdv_netaddr const   addr            = task->context.addr;
    mdv_channel_t const channel_type    = task->context.channel_type;
    char addr_str[MDV_ADDR_LEN_MAX];
    char const         *str_addr        = mdv_netaddr2str(&addr, addr_str, sizeof addr_str);

    mdv_threadpool_remove(threadpool, fd);

    if (events & MDV_EPOLLERR)
    {
        MDV_LOGI("Connection to '%s' failed", str_addr ? str_addr : "???");
        mdv_socket_close(fd);
        mdv_chaman_dialer_state(chaman, &addr, MDV_DIALER_DISCONNECTED);
    }
    else if (events & MDV_EPOLLOUT)
    {
        MDV_LOGI("Connection to '%s' established", str_addr ? str_addr : "???");

        if (chaman->config.channel.handshake(fd, chaman->config.userdata) != MDV_OK
            || mdv_chaman_selector_reg(chaman, fd, &addr, MDV_CHOUT) != MDV_OK)
        {
            mdv_socket_close(fd);
            mdv_chaman_dialer_state(chaman, &addr, MDV_DIALER_DISCONNECTED);
        }
    }
}


static void mdv_chaman_recv_handler(uint32_t events, mdv_threadpool_task_base *task_base)
{
    mdv_recv_task *task = (mdv_recv_task*)task_base;
    mdv_descriptor fd = task->fd;
    mdv_chaman *chaman = task->context.chaman;
    mdv_threadpool *threadpool = chaman->threadpool;
    mdv_channel *channel = task->context.channel;

    mdv_errno err = mdv_channel_recv(channel);

    if ((events & MDV_EPOLLERR) == 0 && (err == MDV_EAGAIN || err == MDV_OK))
    {
        err = mdv_threadpool_rearm(threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLIN | MDV_EPOLLERR, (mdv_threadpool_task_base*)task);
        if (err != MDV_OK)
        {
            char err_msg[128];
            MDV_LOGE("Connection modification failed with error '%s' (%u)",
                                mdv_strerror(err, err_msg, sizeof err_msg), err);
        }
    }
    else
    {
        MDV_LOGI("Peer %p disconnected", fd);

        if(mdv_mutex_lock(&chaman->mutex) == MDV_OK)
        {
            mdv_hashmap_erase(chaman->channels, mdv_channel_id(channel));
            mdv_channel_release(channel);

            mdv_dialer *dialer = mdv_hashmap_find(chaman->dialers, &task->context.addr);
            if(dialer)
                dialer->state = MDV_DIALER_DISCONNECTED;

            mdv_mutex_unlock(&chaman->mutex);
        }

        mdv_threadpool_remove(threadpool, fd);
        mdv_socket_close(fd);
    }
}


static void mdv_chaman_timer_handler(uint32_t events, mdv_threadpool_task_base *task_base)
{
    mdv_timer_task *task = (mdv_timer_task*)task_base;
    mdv_descriptor fd = task->fd;
    mdv_chaman *chaman = task->context.chaman;
    mdv_threadpool *threadpool = chaman->threadpool;

    (void)mdv_skip(fd, sizeof(uint64_t));

    if (mdv_mutex_lock(&chaman->mutex) == MDV_OK)
    {
        mdv_hashmap_foreach(chaman->dialers, mdv_dialer, dialer)
        {
            size_t const time = mdv_gettime();

            if(dialer->state == MDV_DIALER_DISCONNECTED
                && dialer->attempt_time + chaman->config.channel.retry_interval < time)
            {
                dialer->state = MDV_DIALER_CONNECTING;
                dialer->attempt_time = time;
                mdv_chaman_connect(chaman, &dialer->addr, dialer->channel_type);
            }
        }

        mdv_mutex_unlock(&chaman->mutex);
    }
}


static void mdv_chaman_new_connection(mdv_chaman *chaman, mdv_descriptor sock, mdv_netaddr const *addr, mdv_channel_t channel_type, mdv_channel_dir dir, mdv_uuid const *channel_id)
{
    mdv_errno err = MDV_FAILED;

    char addr_str[MDV_ADDR_LEN_MAX];

    char const *str_addr = mdv_netaddr2str(addr, addr_str, sizeof addr_str);

    mdv_channel *channel = 0;

    if(mdv_mutex_lock(&chaman->mutex) == MDV_OK)
    {
        mdv_channel_ref *ref = mdv_hashmap_find(chaman->channels, channel_id);
        mdv_dialer *dialer = mdv_hashmap_find(chaman->dialers, addr);

        if(ref)
        {
            err = MDV_EEXIST;

            if (dialer)
            {
                dialer->state = MDV_DIALER_CONNECTED;
                dialer->channel_id = *channel_id;
            }

            mdv_socket_close(sock);
        }
        else
        {
            channel = chaman->config.channel.create(sock,
                                                    chaman->config.userdata,
                                                    channel_type,
                                                    dir,
                                                    channel_id);

            if (channel)
            {
                mdv_channel_ref channel_ref =
                {
                    .id = *channel_id,
                    .channel = channel,
                    .dialer = dialer
                };

                if (mdv_hashmap_insert(chaman->channels, &channel_ref, sizeof channel_ref))
                {
                    if (dialer)
                    {
                        dialer->state = MDV_DIALER_CONNECTED;
                        dialer->channel_id = *channel_id;
                    }

                    err = MDV_OK;
                }
                else
                {
                    MDV_LOGE("Channel registration failed for '%s'", str_addr ? str_addr : "???");
                    mdv_channel_release(channel);
                    mdv_socket_close(sock);
                    channel = 0;
                }
            }
            else
            {
                MDV_LOGE("Channel creation failed for '%s'", str_addr ? str_addr : "???");
                mdv_socket_close(sock);
            }
        }

        mdv_mutex_unlock(&chaman->mutex);
    }

    if (err == MDV_OK)
    {
        assert(channel);

        mdv_recv_task task =
        {
            .fd = sock,
            .fn = mdv_chaman_recv_handler,
            .context_size = sizeof(mdv_recv_context),
            .context =
            {
                .type = MDV_CT_RECV,
                .chaman = chaman,
                .addr = *addr,
                .channel = channel
            }
        };

        if (!mdv_threadpool_add(chaman->threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLIN | MDV_EPOLLERR, (mdv_threadpool_task_base const *)&task))
        {
            MDV_LOGE("Connection with '%s' registration failed", str_addr ? str_addr : "???");

            if(mdv_mutex_lock(&chaman->mutex) == MDV_OK)
            {
                mdv_hashmap_erase(chaman->channels, channel_id);
                mdv_channel_release(channel);

                mdv_dialer *dialer = mdv_hashmap_find(chaman->dialers, addr);
                if(dialer)
                    dialer->state = MDV_DIALER_DISCONNECTED;

                mdv_mutex_unlock(&chaman->mutex);
            }

            mdv_socket_close(sock);
        }
        else
            MDV_LOGI("New connection with '%s' successfully registered", str_addr ? str_addr : "???");
    }
}


static void mdv_chaman_select_handler(uint32_t events, mdv_threadpool_task_base *task_base)
{
    mdv_selector_task *selector_task = (mdv_selector_task*)task_base;
    mdv_chaman *chaman = selector_task->context.chaman;
    mdv_descriptor fd = selector_task->fd;
    mdv_netaddr const addr = selector_task->context.addr;
    mdv_channel_dir const dir = selector_task->context.dir;

    mdv_channel_t type = 0;
    mdv_uuid channel_id;

    mdv_errno err = chaman->config.channel.accept(fd, chaman->config.userdata, &type, &channel_id);

    if ((events & MDV_EPOLLERR) == 0 && err == MDV_EAGAIN)
    {
        err = mdv_threadpool_rearm(chaman->threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLIN | MDV_EPOLLERR, (mdv_threadpool_task_base*)selector_task);

        if (err != MDV_OK)
        {
            char err_msg[128];
            MDV_LOGE("Connection modification failed with error '%s' (%u)", mdv_strerror(err, err_msg, sizeof err_msg), err);
            mdv_threadpool_remove(chaman->threadpool, fd);
            mdv_socket_close(fd);
        }
    }
    else
    {
        mdv_threadpool_remove(chaman->threadpool, fd);

        if (err == MDV_OK)
            mdv_chaman_new_connection(chaman, fd, &addr, type, dir, &channel_id);
        else
        {
            char addr_str[MDV_ADDR_LEN_MAX];
            char const *str_addr = mdv_netaddr2str(&addr, addr_str, sizeof addr_str);
            MDV_LOGE("Channel selection failed for %s", str_addr ? str_addr : "???");
            mdv_socket_close(fd);
        }
    }
}


static mdv_errno mdv_chaman_selector_reg(mdv_chaman         *chaman,
                                         mdv_descriptor      fd,
                                         mdv_netaddr const  *addr,
                                         mdv_channel_dir     dir)
{
    mdv_selector_task selector_task =
    {
        .fd = fd,
        .fn = mdv_chaman_select_handler,
        .context_size = sizeof(mdv_selector_context),
        .context =
        {
            .type = MDV_CT_SELECTOR,
            .chaman = chaman,
            .addr = *addr,
            .dir = dir
        }
    };

    if (!mdv_threadpool_add(chaman->threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLIN | MDV_EPOLLERR, (mdv_threadpool_task_base const *)&selector_task))
    {
        char addr_str[MDV_ADDR_LEN_MAX];
        char const *str_addr = mdv_netaddr2str(addr, addr_str, sizeof addr_str);
        MDV_LOGE("Connection '%s' accepting failed", str_addr ? str_addr : "???");
        return MDV_FAILED;
    }

    return MDV_OK;
}


static void mdv_chaman_accept_handler(uint32_t events, mdv_threadpool_task_base *task_base)
{
    mdv_listener_task *listener_task = (mdv_listener_task*)task_base;

    mdv_netaddr addr =
    {
        .sock.type = MDV_SOCK_STREAM
    };

    mdv_descriptor sock = mdv_socket_accept(listener_task->fd, &addr.sock.addr);

    mdv_chaman *chaman = listener_task->context.chaman;

    if (sock != MDV_INVALID_DESCRIPTOR)
    {
        mdv_socket_nonblock(sock);
        mdv_socket_tcp_keepalive(sock, chaman->config.channel.keepidle,
                                       chaman->config.channel.keepcnt,
                                       chaman->config.channel.keepintvl);

        if (chaman->config.channel.handshake(sock, chaman->config.userdata) != MDV_OK
            || mdv_chaman_selector_reg(chaman, sock, &addr, MDV_CHIN) != MDV_OK)
        {
            mdv_socket_close(sock);
        }
    }
}


mdv_errno mdv_chaman_listen(mdv_chaman *chaman, char const *addr)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    mdv_netaddr netaddr;

    mdv_errno err = mdv_str2netaddr(addr, &netaddr);

    if (err != MDV_OK)
    {
        char err_msg[128];
        MDV_LOGE("Address resolution failed with error '%s' (%d)", mdv_strerror(err, err_msg, sizeof err_msg), err);
        mdv_rollback(rollbacker);
        return err;
    }


    mdv_descriptor sd = mdv_socket(netaddr.sock.type);

    if(sd == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("Listener socket '%s' not created", addr);
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_socket_reuse_addr(sd);
    mdv_socket_nonblock(sd);
    mdv_socket_tcp_keepalive(sd, chaman->config.channel.keepidle,
                                 chaman->config.channel.keepcnt,
                                 chaman->config.channel.keepintvl);
    mdv_socket_tcp_defer_accept(sd);


    mdv_rollbacker_push(rollbacker, mdv_socket_close, sd);

    err = mdv_socket_bind(sd, &netaddr.sock.addr);

    if(err != MDV_OK)
    {
        char err_msg[128];
        MDV_LOGE("Binding to address '%s' failed with error '%s' (%u)",
                        addr, mdv_strerror(err, err_msg, sizeof err_msg), err);
        mdv_rollback(rollbacker);
        return err;
    }


    err = mdv_socket_listen(sd);

    if(err != MDV_OK)
    {
        char err_msg[128];
        MDV_LOGE("Address '%s' listening failed with error '%s' (%u)",
                        addr, mdv_strerror(err, err_msg, sizeof err_msg), err);
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
        MDV_LOGE("Address '%s' listening failed", addr);
        mdv_rollback(rollbacker);
        return err;
    }

    mdv_rollbacker_free(rollbacker);

    return MDV_OK;
}


static mdv_errno mdv_chaman_dialer_reg(mdv_chaman         *chaman,
                                       mdv_netaddr const  *netaddr,
                                       mdv_channel_t       channel_type,
                                       mdv_dialer        **dialer)
{
    mdv_errno err = MDV_FAILED;

    *dialer = 0;

    if(mdv_mutex_lock(&chaman->mutex) == MDV_OK)
    {
        mdv_dialer new_dialer =
        {
            .addr         = *netaddr,
            .state        = MDV_DIALER_CONNECTING,
            .attempt_time = mdv_gettime(),
            .channel_type = channel_type
        };

        *dialer = mdv_hashmap_find(chaman->dialers, netaddr);

        if (!*dialer)
        {
            *dialer = mdv_hashmap_insert(chaman->dialers, &new_dialer, sizeof new_dialer);

            if(*dialer)
                err = MDV_OK;
        }
        else
        {
            *dialer = 0;
            err = MDV_EEXIST;
        }

        mdv_mutex_unlock(&chaman->mutex);
    }

    return err;
}


static void mdv_chaman_dialer_unreg(mdv_chaman *chaman, mdv_netaddr const *addr)
{
    if(mdv_mutex_lock(&chaman->mutex) == MDV_OK)
    {
        if (!mdv_hashmap_erase(chaman->dialers, addr))
            MDV_LOGE("Dialer not found");
        mdv_mutex_unlock(&chaman->mutex);
    }
    else
        MDV_LOGE("Dialer unregistration failed");
}


static void mdv_chaman_dialer_state(mdv_chaman *chaman, mdv_netaddr const *addr, mdv_dialer_state state)
{
    if(mdv_mutex_lock(&chaman->mutex) == MDV_OK)
    {
        mdv_dialer *dialer = mdv_hashmap_find(chaman->dialers, addr);
        if (dialer)
            dialer->state = state;
        else
            MDV_LOGE("Dialer not found");
        mdv_mutex_unlock(&chaman->mutex);
    }
    else
        MDV_LOGE("dialer_discard failed");
}


static mdv_errno mdv_chaman_connect(mdv_chaman *chaman, mdv_netaddr const *netaddr, mdv_channel_t channel_type)
{
    char addr_str[MDV_ADDR_LEN_MAX];

    char const *addr = mdv_netaddr2str(netaddr, addr_str, sizeof addr_str);

    MDV_LOGD("Connecting to '%s'", addr ? addr : "???");

    mdv_descriptor sd = mdv_socket(netaddr->sock.type);

    if(sd == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("Socket creation failed");
        return MDV_FAILED;
    }

    mdv_socket_nonblock(sd);
    mdv_socket_tcp_keepalive(sd, chaman->config.channel.keepidle,
                                 chaman->config.channel.keepcnt,
                                 chaman->config.channel.keepintvl);

    mdv_errno err = mdv_socket_connect(sd, &netaddr->sock.addr);

    if (err != MDV_INPROGRESS)
    {
        char err_msg[128];
        MDV_LOGW("Connection to %s failed with error '%s' (%d)",
            addr ? addr : "???", mdv_strerror(err, err_msg, sizeof err_msg), err);
        mdv_socket_close(sd);
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
            .addr         = *netaddr,
            .channel_type = channel_type
        }
    };

    if (!mdv_threadpool_add(chaman->threadpool, MDV_EPOLLET | MDV_EPOLLONESHOT | MDV_EPOLLOUT | MDV_EPOLLERR, (mdv_threadpool_task_base const *)&task))
    {
        MDV_LOGW("Connection to %s failed", addr ? addr : "???");
        mdv_socket_close(sd);
        return MDV_FAILED;
    }

    return MDV_OK;
}


mdv_errno mdv_chaman_dial(mdv_chaman *chaman, char const *addr, uint8_t type)
{
    mdv_netaddr netaddr;

    mdv_errno err = mdv_str2netaddr(addr, &netaddr);

    if (err != MDV_OK)
    {
        char err_msg[128];
        MDV_LOGE("Address resolution failed with error '%s' (%d)",
                        mdv_strerror(err, err_msg, sizeof err_msg), err);
        return err;
    }

    mdv_dialer *dialer = 0;

    err = mdv_chaman_dialer_reg(chaman, &netaddr, type, &dialer);

    switch(err)
    {
        case MDV_EEXIST:
        {
            return MDV_OK;
        }

        case MDV_FAILED:
        {
            MDV_LOGE("New dialer registration failed");
            return err;
        }

        default:
            break;
    }

    err = mdv_chaman_connect(chaman, &netaddr, type);

    if (err != MDV_OK)
    {
        MDV_LOGE("Connection to %s failed", addr);
        mdv_chaman_dialer_unreg(chaman, &netaddr);
        return err;
    }

    return MDV_OK;
}
