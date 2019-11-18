#include "mdv_client.h"
#include "mdv_messages.h"
#include "mdv_channel.h"
#include <mdv_version.h>
#include <mdv_alloc.h>
#include <mdv_binn.h>
#include <mdv_log.h>
#include <mdv_msg.h>
#include <mdv_rollbacker.h>
#include <mdv_ctypes.h>
#include <mdv_chaman.h>
#include <mdv_socket.h>
#include <mdv_mutex.h>
#include <mdv_threads.h>
#include <signal.h>


static               int total_connections = 0;
static _Thread_local int thread_connections = 0;


static void mdv_client_init()
{
    static bool is_init = false;

    if (!is_init)
    {
        is_init = true;
        signal(SIGPIPE, SIG_IGN);
        //mdv_logf_set_level(ZF_LOG_VERBOSE);
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
    mdv_chaman         *chaman;             ///< Channels manager
    mdv_mutex           mutex;              ///< Mutex for user guard
    mdv_channel        *channel;            ///< Connection context
    uint32_t            response_timeout;   ///< Temeout for responses (in milliseconds)
};


/// @endcond


static mdv_channel * mdv_client_channel_retain(mdv_client *client)
{
    mdv_channel *channel = 0;

    if (mdv_mutex_lock(&client->mutex) == MDV_OK)
    {
        if (client->channel)
            channel = mdv_channel_retain(client->channel);
        mdv_mutex_unlock(&client->mutex);
    }

    return channel;
}


static bool mdv_client_channel_set(mdv_client *client, mdv_channel *channel)
{
    bool ret = false;

    if (mdv_mutex_lock(&client->mutex) == MDV_OK)
    {
        mdv_channel_release(client->channel);
        client->channel = channel;
        ret = true;
        mdv_mutex_unlock(&client->mutex);
    }

    return ret;
}


static mdv_errno mdv_client_send(mdv_client *client, mdv_msg *req, mdv_msg *resp, size_t timeout)
{
    mdv_channel *channel = mdv_client_channel_retain(client);

    if (!channel)
        return MDV_FAILED;

    mdv_errno err = mdv_channel_send(channel, req, resp, timeout);

    mdv_channel_release(channel);

    return err;
}


static mdv_errno mdv_client_conctx_select(mdv_descriptor fd, uint8_t *type)
{
    uint8_t tag;
    size_t len = sizeof tag;

    mdv_errno err = mdv_read(fd, &tag, &len);

    if (err == MDV_OK)
        *type = tag;
    else
        MDV_LOGE("Channel selection failed with error %d", err);

    return err;
}


static void * mdv_client_conctx_create(mdv_descriptor    fd,
                                       mdv_string const *addr,
                                       void             *userdata,
                                       uint8_t           type,
                                       mdv_channel_dir   dir)
{
    (void)addr;

    mdv_client *client = userdata;

    mdv_errno err = mdv_write_all(fd, &type, sizeof type);

    if (err != MDV_OK)
    {
        MDV_LOGE("Channel tag was not sent");
        mdv_socket_shutdown(fd, MDV_SOCK_SHUT_RD | MDV_SOCK_SHUT_WR);
        return 0;
    }

    mdv_channel *channel = mdv_channel_create(fd);

    if (!channel)
    {
        MDV_LOGE("Connection failed");
        return 0;
    }

    if (!mdv_client_channel_set(client, channel))
    {
        mdv_channel_release(channel);
        channel = 0;
    }

    return channel;
}


static mdv_errno mdv_client_conctx_recv(void *userdata, void *ctx)
{
    (void)userdata;
    return mdv_channel_recv((mdv_channel *)ctx);
}


static void mdv_client_conctx_closed(void *userdata, void *ctx)
{
    mdv_client *client = userdata;
    mdv_client_channel_set(client, 0);
    // mdv_user_release((mdv_user *)ctx);
}


static mdv_errno mdv_client_table_info_handler(mdv_msg const *msg, mdv_msg_table_info *table_info)
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


static mdv_errno mdv_client_status_handler(mdv_msg const *msg, mdv_errno *err)
{
    mdv_msg_status status;

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
        return MDV_FAILED;

    if (!mdv_unbinn_status(&binn_msg, &status))
    {
        MDV_LOGE("Invalid status");
        binn_free(&binn_msg);
        return MDV_FAILED;
    }

    *err = (mdv_errno)status.err;

    binn_free(&binn_msg);

    return MDV_OK;
}


static mdv_errno mdv_client_topology_handler(mdv_msg const *msg, mdv_topology **topology)
{
    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
        return MDV_FAILED;

    *topology = mdv_unbinn_topology(&binn_msg);

    if (!*topology)
    {
        MDV_LOGE("Invalid topology");
        binn_free(&binn_msg);
        return MDV_FAILED;
    }

    binn_free(&binn_msg);

    return MDV_OK;
}


static mdv_errno  mdv_client_row_info_handler(mdv_msg const * msg, mdv_msg_row_info *info)
{
    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
        return MDV_FAILED;

    if (!mdv_unbinn_row_info(&binn_msg, info))
    {
        MDV_LOGE("Invalid row info");
        binn_free(&binn_msg);
        return MDV_FAILED;
    }

    binn_free(&binn_msg);

    return MDV_OK;
}


mdv_client * mdv_client_connect(mdv_client_config const *config)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    mdv_client_init();

    mdv_rollbacker_push(rollbacker, mdv_client_finalize);

    // Allocate memory for client

    mdv_client *client = mdv_alloc(sizeof(mdv_client), "client");

    if (!client)
    {
        MDV_LOGE("No memory for new client");
        mdv_rollback(rollbacker);
        return 0;
    }

    memset(client, 0, sizeof *client);

    mdv_rollbacker_push(rollbacker, mdv_free, client, "client");

    client->response_timeout = config->connection.response_timeout * 1000;

    // Mutex
    mdv_errno err = mdv_mutex_create(&client->mutex);

    if (err != MDV_OK)
    {
        MDV_LOGE("Mutexcreation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &client->mutex);

    // Channels manager
    mdv_chaman_config const chaman_config =
    {
        .channel =
        {
            .keepidle   = config->connection.keepidle,
            .keepcnt    = config->connection.keepcnt,
            .keepintvl  = config->connection.keepintvl,
            .select     = mdv_client_conctx_select,
            .create     = mdv_client_conctx_create,
            .recv       = mdv_client_conctx_recv,
            .close      = mdv_client_conctx_closed
        },
        .threadpool =
        {
            .size = config->threadpool.size,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .userdata = client
    };

    client->chaman = mdv_chaman_create(&chaman_config);

    if (!client->chaman)
    {
        MDV_LOGE("Channels manager creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_chaman_free, client->chaman);

    // Connect to server
    err = mdv_chaman_connect(client->chaman, mdv_str((char*)config->db.addr), MDV_CTX_USER);

    if (err != MDV_OK)
    {
        MDV_LOGE("Connection to '%s' failed with error: %s (%d)", config->db.addr, mdv_strerror(err), err);
        mdv_rollback(rollbacker);
        return 0;
    }

    // Wait connection
    mdv_channel *channel = mdv_client_channel_retain(client);

    for(uint32_t tiemout = 0;
        !channel && tiemout < client->response_timeout;
        channel = mdv_client_channel_retain(client))
    {
        tiemout += 100;
        mdv_sleep(100);
    }

    if (!channel
       || mdv_channel_wait_connection(channel, client->response_timeout) != MDV_OK)
    {
        MDV_LOGE("Connection to '%s' failed", config->db.addr);
        mdv_channel_release(channel);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_channel_release(channel);

    mdv_rollbacker_free(rollbacker);

    return client;
}


void mdv_client_close(mdv_client *client)
{
    if (client)
    {
        mdv_chaman_free(client->chaman);
        mdv_channel_release(client->channel);
        mdv_mutex_free(&client->mutex);
        mdv_free(client, "client");
        mdv_client_finalize();
    }
}


mdv_errno mdv_create_table(mdv_client *client, mdv_table_desc *table)
{
    mdv_msg_create_table create_table =
    {
        .table = table
    };

    binn create_table_msg;

    if (!mdv_binn_create_table(&create_table, &create_table_msg))
        return MDV_FAILED;

    mdv_msg req =
    {
        .hdr =
        {
            .id   = mdv_msg_create_table_id,
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
            case mdv_message_id(table_info):
            {
                mdv_msg_table_info info;

                err = mdv_client_table_info_handler(&resp, &info);

                if (err == MDV_OK)
                    table->id = info.id;

                break;
            }

            case mdv_message_id(status):
            {
                if (mdv_client_status_handler(&resp, &err) == MDV_OK)
                    break;
                // fallthrough
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


mdv_errno mdv_get_topology(mdv_client *client, mdv_topology **topology)
{
    *topology = 0;

    mdv_msg_get_topology get_topology = {};

    binn binn_msg;

    if (!mdv_binn_get_topology(&get_topology, &binn_msg))
        return MDV_FAILED;

    mdv_msg req =
    {
        .hdr =
        {
            .id   = mdv_msg_get_topology_id,
            .size = binn_size(&binn_msg)
        },
        .payload = binn_ptr(&binn_msg)
    };

    mdv_msg resp;

    mdv_errno err = mdv_client_send(client, &req, &resp, client->response_timeout);

    binn_free(&binn_msg);

    if (err == MDV_OK)
    {
        switch(resp.hdr.id)
        {
            case mdv_message_id(topology):
            {
                err = mdv_client_topology_handler(&resp, topology);
                break;
            }

            case mdv_message_id(status):
            {
                if (mdv_client_status_handler(&resp, &err) == MDV_OK)
                    break;
                // fallthrough
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


mdv_errno mdv_insert_row(mdv_client *client, mdv_gobjid const *table_id, mdv_field const *fields, mdv_row_base const *row, mdv_gobjid *id)
{
    mdv_msg_insert_row row_msg;

    row_msg.table = *table_id;
    row_msg.row = row;

    binn insert_row_msg;

    if (!mdv_binn_insert_row(&row_msg, fields, &insert_row_msg))
        return MDV_FAILED;


    mdv_msg req =
    {
        .hdr =
            {
                .id   = mdv_msg_insert_row_id,
                .size = binn_size(&insert_row_msg)
            },
        .payload = binn_ptr(&insert_row_msg)
    };

    mdv_msg resp;

    mdv_errno err = mdv_client_send(client, &req, &resp, client->response_timeout);

    binn_free(&insert_row_msg);

    if (err == MDV_OK)
    {
        switch(resp.hdr.id)
        {
            case mdv_message_id(table_info):
            {
                mdv_msg_row_info info;
                err = mdv_client_row_info_handler(&resp, &info);
                if (err == MDV_OK)
                    *id = info.id;
                break;
            }

            case mdv_message_id(status):
            {
                if (mdv_client_status_handler(&resp, &err) == MDV_OK)
                    break;
                // fallthrough
            }

            default:
                err = MDV_FAILED;
                        MDV_LOGE("Unexpected response");
                break;
        }

        mdv_free_msg(&resp);
    }
}
