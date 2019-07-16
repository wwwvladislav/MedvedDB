#include "mdv_client.h"
#include "mdv_messages.h"
#include <mdv_version.h>
#include <mdv_alloc.h>
#include <mdv_binn.h>
#include <mdv_chaman.h>
#include <mdv_rollbacker.h>
#include <mdv_threads.h>
#include <mdv_timerfd.h>
#include <mdv_socket.h>
#include <mdv_dispatcher.h>
#include <mdv_log.h>
#include <mdv_msg.h>
#include <mdv_condvar.h>
#include <signal.h>
#include <string.h>


static               int total_connections = 0;
static _Thread_local int thread_connections = 0;


static void mdv_client_init()
{
    static bool is_init = false;

    if (!is_init)
    {
        is_init = true;
        signal(SIGPIPE, SIG_IGN);
        // mdv_logf_set_level(ZF_LOG_VERBOSE);
    }

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


static void mdv_client_finalize()
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


/// Client descriptor
struct mdv_client
{
    mdv_uuid            uuid;               ///< server uuid
    mdv_condvar        *connected;          ///< connection status
    mdv_dispatcher     *dispatcher;         ///< Messages dispatcher
    mdv_chaman         *chaman;             ///< channels manager
    uint32_t            response_timeout;   ///< Temeout for responses (in milliseconds)
    mdv_descriptor      sock;               ///< Client socket
};


static mdv_errno mdv_client_send(mdv_client *client, mdv_msg *req, mdv_msg *resp, size_t timeout);
static mdv_errno mdv_client_post(mdv_client *client, mdv_msg *msg);

/// @endcond

/**
 * @brief   Handle incoming HELLO message
 */
static mdv_errno mdv_client_wave_handler(mdv_msg const *msg, void *arg)
{
    MDV_LOGI("<<<<< '%s'", mdv_msg_name(msg->hdr.id));

    mdv_client *client = arg;

    if (msg->hdr.id != mdv_msg_hello_id)
        return MDV_FAILED;

    mdv_msg_hello hello = {};

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
        return MDV_FAILED;

    if (!mdv_unbinn_hello(&binn_msg, &hello))
    {
        MDV_LOGE("Invalid HELLO message");
        binn_free(&binn_msg);
        return MDV_FAILED;
    }

    if(hello.version != MDV_VERSION)
    {
        MDV_LOGE("Invalid client version");
        binn_free(&binn_msg);
        return MDV_FAILED;
    }

    client->uuid = hello.uuid;

    return MDV_OK;
}



static mdv_errno mdv_client_status_handler(mdv_client *client, mdv_msg const *msg)
{
    return MDV_NO_IMPL;
}


static mdv_errno mdv_client_table_info_handler(mdv_client *client, mdv_msg const *msg, mdv_msg_table_info *table_info)
{
    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
        return MDV_FAILED;

    if (!mdv_unbinn_table_info(&binn_msg, table_info))
    {
        MDV_LOGE("Invalid table information");
        binn_free(&binn_msg);
        return MDV_FAILED;
    }

    binn_free(&binn_msg);

    return MDV_OK;
}


static mdv_errno mdv_client_tag(mdv_client *client)
{
    mdv_msg_tag tag =
    {
        .tag = MDV_CLI_USER
    };

    return mdv_dispatcher_write_raw(client->dispatcher, &tag, sizeof tag);
}


/**
 * @brief   Say Hey! to server
 */
static mdv_errno mdv_client_wave(mdv_client *client, size_t timeout)
{
    mdv_msg_hello const hello =
    {
        .uuid    = {},
        .version = MDV_VERSION
    };

    binn hey;

    if (!mdv_binn_hello(&hello, &hey))
        return MDV_FAILED;

    mdv_msg message =
    {
        .hdr =
        {
            .id = mdv_message_id(hello),
            .size = binn_size(&hey)
        },
        .payload = binn_ptr(&hey)
    };

    mdv_msg resp;

    mdv_errno err = mdv_client_send(client, &message, &resp, timeout);

    binn_free(&hey);

    if (err == MDV_OK)
    {
        err = mdv_client_wave_handler(&resp, client);
        mdv_free_msg(&resp);
    }

    return err;
}


/**
 * @brief The response is required.
 */
static mdv_errno mdv_client_send(mdv_client *client, mdv_msg *req, mdv_msg *resp, size_t timeout)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(req->hdr.id));
    return mdv_dispatcher_send(client->dispatcher, req, resp, timeout);
}


/**
 * @brief Send message but response isn't needed.
 */
static mdv_errno mdv_client_post(mdv_client *client, mdv_msg *msg)
{
    MDV_LOGI(">>>>> '%s'", mdv_msg_name(msg->hdr.id));
    return mdv_dispatcher_post(client->dispatcher, msg);
}


static void * mdv_channel_create(mdv_descriptor fd, mdv_string const *addr, void *userdata, uint32_t type, mdv_channel_dir dir)
{
    mdv_client *client = userdata;
    (void)addr;
    (void)type;
    (void)dir;

    client->sock = fd;

    mdv_dispatcher_set_fd(client->dispatcher, fd);

    mdv_errno err = mdv_condvar_signal(client->connected);

    if (err != MDV_OK)
    {
        MDV_LOGE("Client connection notification failed");
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
    }

    return client;
}


static mdv_errno mdv_channel_recv(void *channel)
{
    mdv_client *client = channel;

    mdv_errno err = mdv_dispatcher_read(client->dispatcher);

    switch(err)
    {
        case MDV_OK:
        case MDV_EAGAIN:
            break;

        default:
            mdv_socket_shutdown(client->sock, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
    }

    return err;
}


static void mdv_channel_close(void *channel)
{
    mdv_client *client = (mdv_client *)channel;
    mdv_dispatcher_close_fd(client->dispatcher);
}


mdv_client * mdv_client_connect(mdv_client_config const *config)
{
    mdv_rollbacker(4) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_client_init();

    // Allocate memory for client

    mdv_client *client = mdv_alloc(sizeof(mdv_client));

    if (!client)
    {
        MDV_LOGE("No memory for new client");
        mdv_client_finalize();
        return 0;
    }

    memset(client, 0, sizeof *client);

    mdv_rollbacker_push(rollbacker, mdv_free, client);

    client->response_timeout = config->connection.response_timeout * 1000;
    client->sock             = MDV_INVALID_DESCRIPTOR;


    // Create conditional variable for connection status waiting

    client->connected = mdv_condvar_create();

    if (!client->connected)
    {
        MDV_LOGE("Conditional variable not created");
        mdv_rollback(rollbacker);
        mdv_client_finalize();
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_condvar_free, client->connected);


    // Create messages dispatcher

    mdv_dispatcher_handler const handlers[] =
    {
        { mdv_msg_hello_id,  &mdv_client_wave_handler, client }
    };

    client->dispatcher = mdv_dispatcher_create(0, sizeof handlers / sizeof *handlers, handlers);

    if (!client->dispatcher)
    {
        MDV_LOGE("Messages dispatcher not created");
        mdv_rollback(rollbacker);
        mdv_client_finalize();
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_dispatcher_free, client->dispatcher);


    // Create channels manager

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
            .create = mdv_channel_create,
            .recv = mdv_channel_recv,
            .close = mdv_channel_close
        }
    };

    client->chaman = mdv_chaman_create(&chaman_config);

    if (!client->chaman)
    {
        MDV_LOGE("Chaman not created");
        mdv_rollback(rollbacker);
        mdv_client_finalize();
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_chaman_free, client->chaman);


    // Connect to server

    mdv_errno err = mdv_chaman_connect(client->chaman, mdv_str((char*)config->db.addr));

    if (err != MDV_OK)
    {
        MDV_LOGE("Connection to '%s' failed with error: %s (%d)", config->db.addr, mdv_strerror(err), err);
        mdv_rollback(rollbacker);
        mdv_client_finalize();
        return 0;
    }

    // Wait connection

    err = mdv_condvar_timedwait(client->connected, config->connection.timeout * 1000);

    if (err != MDV_OK)
    {
        MDV_LOGE("Connection to '%s' failed", config->db.addr);
        mdv_rollback(rollbacker);
        mdv_client_finalize();
        return 0;
    }

    err = mdv_client_tag(client);

    if (err != MDV_OK)
    {
        MDV_LOGE("Connection to '%s' failed", config->db.addr);
        mdv_rollback(rollbacker);
        mdv_client_finalize();
        return 0;
    }

    err = mdv_client_wave(client, client->response_timeout);

    if (err != MDV_OK)
    {
        MDV_LOGE("Connection to '%s' failed", config->db.addr);
        mdv_rollback(rollbacker);
        mdv_client_finalize();
        return 0;
    }

    return client;
}


void mdv_client_close(mdv_client *client)
{
    if (client)
    {
        mdv_chaman_free(client->chaman);
        mdv_dispatcher_free(client->dispatcher);
        mdv_condvar_free(client->connected);
        mdv_free(client);
        mdv_client_finalize();
    }
}


mdv_errno mdv_create_table(mdv_client *client, mdv_table_base *table)
{
    mdv_msg_create_table_base *create_table = (mdv_msg_create_table_base *)table;

    binn create_table_msg;

    if (!mdv_binn_create_table(create_table, &create_table_msg))
        return MDV_FAILED;

    mdv_msg req =
    {
        .hdr =
        {
            .id = mdv_msg_create_table_id,
            .size = binn_size(&create_table_msg)
        },
        .payload = binn_ptr(&create_table_msg)
    };

    mdv_msg resp;

    mdv_errno err = mdv_client_send(client, &req, &resp, client->response_timeout);

    binn_free(&create_table_msg);

    if (err == MDV_OK)
    {
        switch(resp.hdr.id)
        {
            case mdv_message_id(status):
            {
                err = mdv_client_status_handler(client, &resp);
                break;
            }

            case mdv_message_id(table_info):
            {
                mdv_msg_table_info info;
                err = mdv_client_table_info_handler(client, &resp, &info);
                if (err == MDV_OK)
                    table->uuid = info.uuid;
                break;
            }

            default:
                err = MDV_FAILED;
                MDV_LOGE("Unexpected response");
                break;
        }

        mdv_free_msg(&resp);
    }

    return err;
}
