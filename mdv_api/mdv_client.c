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
#include <mdv_serialization.h>
#include <mdv_router.h>
#include <signal.h>


enum { MDV_FETCH_SIZE = 64 };       // limit for rows fetching


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

    if (!mdv_msg_table_info_unbinn(&binn_msg, table_info))
    {
        MDV_LOGE("Invalid table information");
        binn_free(&binn_msg);
        return MDV_FAILED;
    }

    binn_free(&binn_msg);

    return MDV_OK;
}


static mdv_table_desc * mdv_client_table_desc_handler(mdv_msg const *msg)
{
    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
        return 0;

    mdv_table_desc *desc = mdv_unbinn_table_desc(&binn_msg);

    binn_free(&binn_msg);

    return desc;
}


static mdv_errno mdv_client_view_handler(mdv_msg const *msg, uint32_t *view_id)
{
    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
        return 0;

    mdv_msg_view view;

    if (!mdv_msg_view_unbinn(&binn_msg, &view))
    {
        MDV_LOGE("Invalid view");
        binn_free(&binn_msg);
        return MDV_FAILED;
    }

    *view_id = view.id;

    binn_free(&binn_msg);

    return MDV_OK;
}


static mdv_errno mdv_client_status_handler(mdv_msg const *msg, mdv_errno *err)
{
    mdv_msg_status status;

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
        return MDV_FAILED;

    if (!mdv_msg_status_unbinn(&binn_msg, &status))
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

    *topology = mdv_msg_topology_unbinn(&binn_msg);

    if (!*topology)
    {
        MDV_LOGE("Invalid topology");
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


mdv_table * mdv_create_table(mdv_client *client, mdv_table_desc *desc)
{
    mdv_msg_create_table create_table =
    {
        .desc = desc
    };

    binn create_table_msg;

    if (!mdv_msg_create_table_binn(&create_table, &create_table_msg))
        return 0;

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

    mdv_table *tbl = 0;

    if (err == MDV_OK)
    {
        switch(resp.hdr.id)
        {
            case mdv_message_id(table_info):
            {
                mdv_msg_table_info info;

                err = mdv_client_table_info_handler(&resp, &info);

                tbl = mdv_table_create(&info.id, desc);

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

    return tbl;
}


mdv_table * mdv_get_table(mdv_client *client, mdv_uuid const *uuid)
{
    mdv_msg_get_table get_table =
    {
        .id = *uuid
    };

    binn get_table_msg;

    if (!mdv_msg_get_table_binn(&get_table, &get_table_msg))
        return 0;

    mdv_msg req =
    {
        .hdr =
        {
            .id   = mdv_msg_get_table_id,
            .size = binn_size(&get_table_msg)
        },
        .payload = binn_ptr(&get_table_msg)
    };

    mdv_msg resp;

    mdv_errno err = mdv_client_send(client, &req, &resp, client->response_timeout);

    binn_free(&get_table_msg);

    mdv_table *tbl = 0;

    if (err == MDV_OK)
    {
        switch(resp.hdr.id)
        {
            case mdv_message_id(table_desc):
            {
                mdv_table_desc *desc = mdv_client_table_desc_handler(&resp);

                if (desc)
                {
                    tbl = mdv_table_create(uuid, desc);
                    mdv_free(desc, "table_desc");
                    break;
                }
                // fallthrough
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

    return tbl;
}


mdv_errno mdv_get_topology(mdv_client *client, mdv_topology **topology)
{
    *topology = 0;

    mdv_msg_get_topology get_topology = {};

    binn binn_msg;

    if (!mdv_msg_get_topology_binn(&get_topology, &binn_msg))
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


mdv_hashmap * mdv_get_routes(mdv_client *client)
{
    mdv_topology *topology = 0;
    mdv_errno err = mdv_get_topology(client, &topology);

    mdv_hashmap *routes = 0;

    if (err == MDV_OK && topology)
    {
        routes = mdv_routes_find(topology, mdv_channel_uuid(client->channel));
        mdv_topology_release(topology);
    }

    return routes;
}


mdv_errno mdv_insert_rows(mdv_client *client, mdv_table *table, mdv_rowset *rowset)
{
    binn serialized_rows;

    if (!mdv_binn_rowset(rowset, &serialized_rows, mdv_table_description(table)))
        return MDV_FAILED;

    mdv_msg_insert_into insert_msg =
    {
        .table = *mdv_table_uuid(table),
        .rows = &serialized_rows
    };

    binn insert_into_msg;

    if (!mdv_msg_insert_into_binn(&insert_msg, &insert_into_msg))
    {
        binn_free(&serialized_rows);
        return MDV_FAILED;
    }

    binn_free(&serialized_rows);

    mdv_msg req =
    {
        .hdr =
        {
            .id   = mdv_msg_insert_into_id,
            .size = binn_size(&insert_into_msg)
        },
        .payload = binn_ptr(&insert_into_msg)
    };

    mdv_msg resp;

    mdv_errno err = mdv_client_send(client, &req, &resp, client->response_timeout);

    binn_free(&insert_into_msg);

    if (err == MDV_OK)
    {
        switch(resp.hdr.id)
        {
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


/// Set of rows enumerator
typedef struct
{
    mdv_enumerator          base;       ///< Base type for rowset enumerator
    uint32_t                view_id;    ///< View identifier
    mdv_client             *client;     ///< Client descriptor
    mdv_table              *table;      ///< Table descriptor
    mdv_rowset             *rowset;     ///< Current fetched rowset
    mdv_enumerator         *it;         ///< Fetched rowset iterator
    mdv_bitset             *fields;     ///< Fields mask for fetching
} mdv_rowset_enumerator_impl;


static mdv_enumerator * mdv_rowset_enumerator_impl_retain(mdv_enumerator *enumerator)
{
    atomic_fetch_add_explicit(&enumerator->rc, 1, memory_order_acquire);
    return enumerator;
}


static uint32_t mdv_rowset_enumerator_impl_release(mdv_enumerator *enumerator)
{
    uint32_t rc = 0;

    if (enumerator)
    {
        mdv_rowset_enumerator_impl *impl = (mdv_rowset_enumerator_impl *)enumerator;

        rc = atomic_fetch_sub_explicit(&enumerator->rc, 1, memory_order_release) - 1;

        if (!rc)
        {
            mdv_table_release(impl->table);
            mdv_rowset_release(impl->rowset);
            mdv_enumerator_release(impl->it);
            mdv_bitset_release(impl->fields);
            memset(impl, 0, sizeof *impl);
            mdv_free(enumerator, "rowset_enumerator");
        }
    }

    return rc;
}


static mdv_errno mdv_rowset_enumerator_impl_reset(mdv_enumerator *enumerator)
{
    return MDV_NO_IMPL;
}


static mdv_errno mdv_rowset_enumerator_impl_fetch_first(mdv_rowset_enumerator_impl *impl)
{
    return MDV_NO_IMPL;
}


static mdv_errno mdv_rowset_enumerator_impl_fetch(mdv_rowset_enumerator_impl *impl)
{
    mdv_client *client = impl->client;

    mdv_msg_fetch const msg =
    {
        .id = impl->view_id
    };

    binn binn_msg;

    if (!mdv_msg_fetch_binn(&msg, &binn_msg))
        return MDV_FAILED;

    mdv_msg req =
    {
        .hdr =
        {
            .id   = mdv_msg_fetch_id,
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
            case mdv_message_id(rowset):
            {
                // TODO
                printf("Rowset!!!");
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


static mdv_errno mdv_rowset_enumerator_impl_next(mdv_enumerator *enumerator)
{
    mdv_rowset_enumerator_impl *impl = (mdv_rowset_enumerator_impl *)enumerator;

    mdv_errno err = mdv_rowset_enumerator_impl_fetch(impl);

/*
    if(!impl->it)
    {
        // Fetch first set of rows
        return mdv_rowset_enumerator_impl_fetch_first(impl);
    }
    else if (mdv_enumerator_next(impl->it) != MDV_OK)
    {
        // Fetch next set of rows
        return mdv_rowset_enumerator_impl_fetch(impl);
    }
    else
        return MDV_OK;
*/

    return err;
}


static void * mdv_rowset_enumerator_impl_current(mdv_enumerator *enumerator)
{
    mdv_rowset_enumerator_impl *impl = (mdv_rowset_enumerator_impl *)enumerator;
    return impl->it ? mdv_enumerator_current(impl->it) : 0;
}


static mdv_errno mdv_select_request(mdv_client *client,
                                    mdv_table  *table,
                                    mdv_bitset *fields,
                                    char const *filter,
                                    uint32_t   *view_id)
{
    mdv_msg_select const select =
    {
        .table  = *mdv_table_uuid(table),
        .fields = fields,
        .filter = filter
    };

    binn select_msg;

    if (!mdv_msg_select_binn(&select, &select_msg))
        return MDV_FAILED;

    mdv_msg req =
    {
        .hdr =
        {
            .id   = mdv_msg_select_id,
            .size = binn_size(&select_msg)
        },
        .payload = binn_ptr(&select_msg)
    };

    mdv_msg resp;

    mdv_errno err = mdv_client_send(client, &req, &resp, client->response_timeout);

    binn_free(&select_msg);

    if (err == MDV_OK)
    {
        switch(resp.hdr.id)
        {
            case mdv_message_id(view):
            {
                err = mdv_client_view_handler(&resp, view_id);
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


mdv_enumerator * mdv_select(mdv_client *client,
                            mdv_table  *table,
                            mdv_bitset *fields,
                            char const *filter)
{
    uint32_t view_id = 0;

    mdv_errno err = mdv_select_request(client,
                                       table,
                                       fields,
                                       filter,
                                       &view_id);

    if (err != MDV_OK)
        return 0;

    mdv_rowset_enumerator_impl *enumerator =
                mdv_alloc(sizeof(mdv_rowset_enumerator_impl),
                        "rowset_enumerator");

    if (!enumerator)
    {
        MDV_LOGE("No memory for rows iterator");
        return 0;
    }

    atomic_init(&enumerator->base.rc, 1);

    static mdv_ienumerator const vtbl =
    {
        .retain  = mdv_rowset_enumerator_impl_retain,
        .release = mdv_rowset_enumerator_impl_release,
        .reset   = mdv_rowset_enumerator_impl_reset,
        .next    = mdv_rowset_enumerator_impl_next,
        .current = mdv_rowset_enumerator_impl_current
    };

    enumerator->base.vptr = &vtbl;

    enumerator->view_id = view_id;
    enumerator->client  = client;
    enumerator->table   = mdv_table_retain(table);
    enumerator->rowset  = 0;
    enumerator->it      = 0;
    enumerator->fields  = mdv_bitset_retain(fields);

    return &enumerator->base;
}
