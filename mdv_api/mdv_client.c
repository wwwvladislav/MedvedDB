#include "mdv_client.h"
#include "mdv_messages.h"
#include "mdv_handler.h"
#include "mdv_protocol.h"
#include "mdv_version.h"
#include "mdv_status.h"
#include <mdv_alloc.h>
#include <mdv_binn.h>
#include <mdv_threads.h>
#include <stdbool.h>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdatomic.h>


// TODO: Implement MT safe client API


static               int total_connections = 0;
static _Thread_local int thread_connections = 0;


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


typedef struct
{
    void *arg;
    int (*fn)(mdv_message, void *);
} mdv_client_handler;


struct mdv_client
{
    atomic_uint_fast32_t    req_id;
    nng_socket              sock;
    nng_dialer              dialer;
    mdv_client_handler      handlers[mdv_msg_count];
    int                     err;
    char                    message[1024];
};


static bool mdv_client_handler_reg(mdv_client *client, uint32_t msg_id, mdv_client_handler handler)
{
    if (msg_id >= sizeof client->handlers / sizeof *client->handlers)
    {
        printf("Client message handler not registered. Message ID %u is invalid.", msg_id);
        return false;
    }

    client->handlers[msg_id] = handler;

    return true;
}


static uint32_t mdv_client_new_msg_id(mdv_client *client)
{
    uint32_t id = atomic_fetch_add_explicit(&client->req_id, 1, memory_order_relaxed) + 1;
    return id;
}


static bool mdv_client_send(mdv_client *client, uint32_t msgid, binn *obj, uint32_t *reqid)
{
    nng_msg * msg;
    int err = nng_msg_alloc(&msg, 0);
    if (err)
    {
        printf("%s (%d)", nng_strerror(err), err);
        return false;
    }

    mdv_msg_hdr hdr =
    {
        .msg_id = msgid,
        .req_id = mdv_client_new_msg_id(client)
    };

    if (0
        || (err = nng_msg_append_u32(msg, hdr.msg_id))
        || (err = nng_msg_append_u32(msg, hdr.req_id))
        || (err = nng_msg_append(msg, binn_ptr(obj), binn_size(obj)))
        || (err = nng_sendmsg(client->sock, msg, 0)))
    {
        printf("%s (%d)", nng_strerror(err), err);
        nng_msg_free(msg);
        return false;
    }

    *reqid = hdr.req_id;

    return true;
}


static bool mdv_client_read(mdv_client *client)
{
    nng_msg *nmsg = 0;

    int err = nng_recvmsg(client->sock, &nmsg, 0);
    if (err)
    {
        printf("%s (%d)", nng_strerror(err), err);
        return false;
    }

    bool ret = false;

    if (nmsg)
    {
        size_t len = nng_msg_len(nmsg);
        uint8_t *body = (uint8_t *)nng_msg_body(nmsg);

        if (len >= sizeof(mdv_msg_hdr) && body)
        {
            mdv_msg_hdr hdr = *(mdv_msg_hdr*)body;
            hdr.msg_id = ntohl(hdr.msg_id);
            hdr.req_id = ntohl(hdr.req_id);

            if (hdr.msg_id > mdv_msg_unknown_id && hdr.msg_id < mdv_msg_count)
            {
                mdv_client_handler handler = client->handlers[hdr.msg_id];

                if(handler.fn)
                {
                    mdv_message request = { hdr.msg_id };

                    if(binn_load(body + sizeof hdr, &request.body))
                    {
                        ret = (handler.fn(request, handler.arg) == MDV_STATUS_OK);
                        binn_free(&request.body);
                    }
                }
            }
        }

        nng_msg_free(nmsg);
    }

    return ret;
}


static int mdv_client_status_handler(mdv_message msg, void *arg)
{
    if (msg.id != mdv_msg_status_id)
        return MDV_STATUS_FAILED;

    mdv_client *client = (mdv_client *)arg;

    mdv_msg_status *status = mdv_unbinn_status(&msg.body);
    if (!status)
        return MDV_STATUS_FAILED;

    client->err = status->err;
    strncpy(client->message, status->message, sizeof client->message);
    client->message[sizeof client->message - 1] = 0;

    if (*status->message)
        printf("%s\n", status->message);

    mdv_free(status);

    return client->err;
}


mdv_client * mdv_client_create(char const *addr)
{
    allocator_init();

    mdv_client *client = mdv_alloc(sizeof(mdv_client));

    atomic_init(&client->req_id, 0);

    mdv_client_handler_reg(client, mdv_msg_status_id, (mdv_client_handler) { client, mdv_client_status_handler });

    int err = nng_req0_open(&client->sock);
    if (err)
    {
        printf("%s (%d)", nng_strerror(err), err);
        mdv_client_disconnect(client);
        return 0;
    }

    err = nng_dial(client->sock, addr, &client->dialer, 0);
    if (err)
    {
        printf("%s (%d)", nng_strerror(err), err);
        mdv_client_disconnect(client);
        return 0;
    }

    return client;
}


bool mdv_client_connect(mdv_client *client)
{
    mdv_msg_hello hello =
    {
        MDV_HELLO_SIGNATURE,
        MDV_VERSION
    };

    binn msg;

    if (!mdv_binn_hello(&hello, &msg))
        return false;

    uint32_t req_id;

    if (!mdv_client_send(client, mdv_msg_hello_id, &msg, &req_id))
    {
        binn_free(&msg);
        return false;
    }

    binn_free(&msg);

    if (!mdv_client_read(client))
        return false;

    return true;
}


void mdv_client_disconnect(mdv_client *client)
{
    if (!client)
        return;
    nng_dialer_close(client->dialer);
    nng_close(client->sock);
    mdv_free(client);
    allocator_finalize();
}


int mdv_client_errno(mdv_client *client)
{
    return client->err;
}


char const * mdv_client_status_msg(mdv_client *client)
{
    return *client->message
                ? client->message
                : mdv_status_message(client->err);
}


bool mdv_create_table(mdv_client *client, mdv_table_base *table)
{
    mdv_msg_create_table_base *create_table = (mdv_msg_create_table_base *)table;

    binn msg;

    if (!mdv_binn_create_table(create_table, &msg))
        return false;

    uint32_t req_id;

    if (!mdv_client_send(client, mdv_msg_create_table_id, &msg, &req_id))
    {
        binn_free(&msg);
        return false;
    }

    binn_free(&msg);

    if (!mdv_client_read(client))
        return false;

    return true;
}
