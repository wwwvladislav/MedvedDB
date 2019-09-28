#include "mdv_client.h"
#include "mdv_messages.h"
#include "mdv_user.h"
#include <mdv_version.h>
#include <mdv_alloc.h>
#include <mdv_binn.h>
#include <mdv_log.h>
#include <mdv_msg.h>
#include <mdv_condvar.h>
#include <mdv_rollbacker.h>
#include <mdv_ctypes.h>
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
    mdv_cluster         cluster;            ///< Cluster manager
    mdv_condvar         connected;          ///< Connection status
    uint32_t            response_timeout;   ///< Temeout for responses (in milliseconds)
    mdv_user_data       userdata;           ///< Data which passed to mdv_user_init() as userdata
};


/// @endcond


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

    // Allocate memory for client

    mdv_client *client = mdv_alloc(sizeof(mdv_client), "client");

    if (!client)
    {
        MDV_LOGE("No memory for new client");
        mdv_rollback(rollbacker);
        mdv_client_finalize();
        return 0;
    }

    memset(client, 0, sizeof *client);

    mdv_rollbacker_push(rollbacker, mdv_free, client, "client");

    client->response_timeout = config->connection.response_timeout * 1000;

    // Create conditional variable for connection status waiting

    if (mdv_condvar_create(&client->connected) != MDV_OK)
    {
        MDV_LOGE("Conditional variable not created");
        mdv_rollback(rollbacker);
        mdv_client_finalize();
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_condvar_free, &client->connected);

    client->userdata.user = 0;
    client->userdata.connected = &client->connected;

    // Cluster manager

    mdv_conctx_config const conctx_configs[] =
    {
        { MDV_CLI_USER, sizeof(mdv_user), &mdv_user_init, &mdv_user_free },
    };

    mdv_cluster_config const cluster_config =
    {
        .uuid = {},
        .channel =
        {
            .keepidle   = config->connection.keepidle,
            .keepcnt    = config->connection.keepcnt,
            .keepintvl  = config->connection.keepintvl
        },
        .threadpool =
        {
            .size = config->threadpool.size,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .conctx =
        {
            .userdata = &client->userdata,
            .size     = sizeof conctx_configs / sizeof *conctx_configs,
            .configs  = conctx_configs
        }
    };

    if (mdv_cluster_create(&client->cluster, &cluster_config) != MDV_OK)
    {
        MDV_LOGE("Cluster manager creation failed");
        mdv_rollback(rollbacker);
        mdv_client_finalize();
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_cluster_free, &client->cluster);


    // Connect to server

    mdv_errno err = mdv_cluster_connect(&client->cluster, mdv_str((char*)config->db.addr), MDV_CLI_USER);

    if (err != MDV_OK)
    {
        MDV_LOGE("Connection to '%s' failed with error: %s (%d)", config->db.addr, mdv_strerror(err), err);
        mdv_rollback(rollbacker);
        mdv_client_finalize();
        return 0;
    }

    // Wait connection

    err = mdv_condvar_timedwait(&client->connected, config->connection.timeout * 1000);

    if (err != MDV_OK)
    {
        MDV_LOGE("Connection to '%s' failed", config->db.addr);
        mdv_rollback(rollbacker);
        mdv_client_finalize();
        return 0;
    }

    mdv_rollbacker_free(rollbacker);

    return client;
}


void mdv_client_close(mdv_client *client)
{
    if (client)
    {
        mdv_cluster_free(&client->cluster);
        mdv_condvar_free(&client->connected);
        mdv_free(client, "client");
        mdv_client_finalize();
    }
}


mdv_errno mdv_create_table(mdv_client *client, mdv_table_base const *table, mdv_growid *id)
{
    mdv_msg_create_table_base *create_table = (mdv_msg_create_table_base *)table;

    binn create_table_msg;

    if (!mdv_binn_create_table(create_table, &create_table_msg))
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

    mdv_errno err = mdv_user_send(client->userdata.user, &req, &resp, client->response_timeout);

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

    mdv_errno err = mdv_user_send(client->userdata.user, &req, &resp, client->response_timeout);

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


mdv_errno mdv_insert_row(mdv_client *client, mdv_growid const *table_id, mdv_field const *fields, mdv_row_base const *row, mdv_growid *id)
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

    mdv_errno err = mdv_user_send(client->userdata.user, &req, &resp, client->response_timeout);

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
