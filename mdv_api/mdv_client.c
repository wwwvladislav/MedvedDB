#include "mdv_client.h"
#include "mdv_messages.h"
#include <mdv_version.h>
#include <mdv_alloc.h>
#include <mdv_binn.h>
#include <string.h>
#include <stdatomic.h>
#include <mdv_chaman.h>
#include <mdv_rollbacker.h>
#include <mdv_threads.h>
#include <mdv_timerfd.h>
#include <mdv_socket.h>
#include <mdv_log.h>
#include <mdv_msg.h>
#include <signal.h>


// TODO: Implement MT safe client API


static               int total_connections = 0;
static _Thread_local int thread_connections = 0;


static void init_signal_handlers()
{
    static bool is_init = false;

    if (!is_init)
    {
        is_init = true;
        signal(SIGPIPE, SIG_IGN);
    }
}


static void allocator_init()
{
    if (total_connections++ == 0)
    {
        mdv_alloc_initialize();
        mdv_binn_set_allocator();
        thread_connections++;
    }
    else if (thread_connections++ == 0)
    {
        mdv_alloc_thread_initialize();
    }
}


static void allocator_finalize()
{
    if (!thread_connections
        || !total_connections)
        return;

    if (--total_connections == 0)
    {
        mdv_alloc_finalize();
        thread_connections--;
    }
    else if (--thread_connections == 0)
    {
        mdv_alloc_thread_finalize();
    }
}

/// @cond Doxygen_Suppress

struct mdv_client
{
    mdv_uuid            uuid;               ///< server uuid
    mdv_chaman         *chaman;             ///< channels manager
    mdv_descriptor      fd;                 ///< socket
    atomic_ushort       id;                 ///< id generator
    atomic_bool         connected;          ///< connectio status
    mdv_msg             message;            ///< incomming message
};


static mdv_errno mdv_client_send(mdv_client *client, mdv_msg const *message);
static mdv_errno mdv_client_recv(mdv_client *client);


static mdv_errno mdv_client_wave(mdv_client *client);
static mdv_errno mdv_client_wave_handler(mdv_client *client, binn const *message);

/// @endcond


/**
 * @brief   Say Hey! to peer node
 */
static mdv_errno mdv_client_wave(mdv_client *client)
{
    mdv_msg_hello const msg =
    {
        .uuid = {},
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
            .number = atomic_fetch_add_explicit(&client->id, 1, memory_order_relaxed),
            .size = binn_size(&hey)
        },
        .payload = binn_ptr(&hey)
    };

    mdv_errno err = mdv_client_send(client, &message);

    binn_free(&hey);

    return err;
}


/**
 * @brief   Handle incoming HELLO message
 */
static mdv_errno mdv_client_wave_handler(mdv_client *client, binn const *message)
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

    client->uuid = hello.uuid;

    atomic_store_explicit(&client->connected, 1, memory_order_release);

    return MDV_OK;
}


static mdv_errno mdv_client_recv(mdv_client *client)
{
    mdv_errno err = mdv_read_msg(client->fd, &client->message);

    if(err == MDV_OK)
    {
        MDV_LOGI("<<<<< '%s'", mdv_msg_name(client->message.hdr.id));

        binn binn_msg;

        if(!binn_load(client->message.payload, &binn_msg))
        {
            MDV_LOGW("Message '%s' discarded", mdv_msg_name(client->message.hdr.id));
            return MDV_FAILED;
        }

        // Message handlers
        switch(client->message.hdr.id)
        {
            case mdv_msg_hello_id:
                err = mdv_client_wave_handler(client, &binn_msg);
                break;

            default:
                MDV_LOGE("Unknown message received");
                err = MDV_FAILED;
                break;
        }

        binn_free(&binn_msg);

        mdv_free_msg(&client->message);
    }

    return err;
}


static mdv_errno mdv_client_send(mdv_client *client, mdv_msg const *message)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(message->hdr.id));
    return mdv_write_msg(client->fd, message);
}


static void mdv_channel_init(void *userdata, void *context, mdv_descriptor fd)
{
    mdv_client *client = (mdv_client *)userdata;
    (void)context;

    client->fd = fd;

    mdv_client_wave(client);
}


static mdv_errno mdv_channel_recv(void *userdata, void *context, mdv_descriptor fd)
{
    mdv_client *client = (mdv_client *)userdata;
    (void)context;

    mdv_errno err = mdv_client_recv(client);

    switch(err)
    {
        case MDV_OK:
        case MDV_EAGAIN:
            break;

        default:
            mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
    }

    return err;
}


static void mdv_channel_close(void *userdata, void *context)
{
    mdv_client *client = (mdv_client *)userdata;
    (void)context;
    (void)client;
}


mdv_client * mdv_client_connect(mdv_client_config const *config)
{
    mdv_rollbacker(2) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    init_signal_handlers();
    allocator_init();

    mdv_client *client = mdv_alloc(sizeof(mdv_client));

    if (!client)
    {
        MDV_LOGE("No memory for new client");
        allocator_finalize();
        return 0;
    }

    memset(client, 0, sizeof *client);

    mdv_rollbacker_push(rollbacker, mdv_free, client);

    mdv_chaman_config const chaman_config =
    {
        .peer =
        {
            .keepidle          = config->connection.keepidle,
            .keepcnt           = config->connection.keepcnt,
            .keepintvl         = config->connection.keepintvl
        },
        .threadpool =
        {
            .size = config->threadpool.size,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .userdata = client,
        .channel =
        {
            .context =
            {
                .size = 0,
                .guardsize = 0
            },
            .init = &mdv_channel_init,
            .recv = &mdv_channel_recv,
            .close = &mdv_channel_close
        }
    };

    client->chaman = mdv_chaman_create(&chaman_config);

    if (!client->chaman)
    {
        MDV_LOGE("Chaman not created");
        mdv_rollback(rollbacker);
        allocator_finalize();
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_chaman_free, client->chaman);

    mdv_errno err = mdv_chaman_connect(client->chaman, mdv_str((char*)config->db.addr));

    if (err != MDV_OK)
    {
        MDV_LOGE("Connection to '%s' failed with error: %s (%d)", config->db.addr, mdv_strerror(err), err);
        mdv_rollback(rollbacker);
        allocator_finalize();
        return 0;
    }

    for(uint32_t t = 0; t < config->connection.timeout * 1000; t += 100)
    {
        if (atomic_load_explicit(&client->connected, memory_order_acquire))
            break;
        mdv_sleep(100);
    }

    if (!client->connected)
    {
        MDV_LOGE("Connection to '%s' failed", config->db.addr);
        mdv_rollback(rollbacker);
        allocator_finalize();
        return 0;
    }

    return client;
}


void mdv_client_close(mdv_client *client)
{
    if (client)
    {
        mdv_chaman_free(client->chaman);
        mdv_free(client);
        allocator_finalize();
    }
}


mdv_errno mdv_create_table(mdv_client *client, mdv_table_base *table)
{
    mdv_msg_create_table_base *create_table = (mdv_msg_create_table_base *)table;

    binn create_table_msg;

    if (!mdv_binn_create_table(create_table, &create_table_msg))
        return MDV_FAILED;

    mdv_msg const message =
    {
        .hdr =
        {
            .id = mdv_msg_create_table_id,
            .number = atomic_fetch_add_explicit(&client->id, 1, memory_order_relaxed),
            .size = binn_size(&create_table_msg)
        },
        .payload = binn_ptr(&create_table_msg)
    };

    mdv_errno err = mdv_client_send(client, &message);

    binn_free(&create_table_msg);

    return err;
}
