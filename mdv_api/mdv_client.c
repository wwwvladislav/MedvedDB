#include "mdv_client.h"
#include "mdv_messages.h"
#include "mdv_handler.h"
#include "mdv_version.h"
#include <mdv_alloc.h>
#include <mdv_binn.h>
#include <stdbool.h>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>


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
    nng_socket          sock;
    nng_dialer          dialer;
    mdv_client_handler  handlers[mdv_msg_count];
    int                 err;
    char                message[1024];
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


static bool mdv_client_send(mdv_client *client, uint32_t id, binn *obj)
{
    nng_msg * msg;
    int err = nng_msg_alloc(&msg, 0);
    if (err)
    {
        printf("%s (%d)", nng_strerror(err), err);
        return false;
    }

    err = nng_msg_append_u32(msg, id);
    if (err)
    {
        printf("%s (%d)", nng_strerror(err), err);
        nng_msg_free(msg);
        return false;
    }

    err = nng_msg_append(msg, binn_ptr(obj), binn_size(obj));
    if (err)
    {
        printf("%s (%d)", nng_strerror(err), err);
        nng_msg_free(msg);
        return false;
    }

    err = nng_sendmsg(client->sock, msg, 0);
    if (err)
    {
        printf("%s (%d)", nng_strerror(err), err);
        nng_msg_free(msg);
        return false;
    }

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

        if (len >= sizeof(uint32_t) && body)
        {
            uint32_t msg_id = ntohl(*(uint32_t*)body);

            if (msg_id > mdv_msg_unknown_id && msg_id < mdv_msg_count)
            {
                mdv_client_handler handler = client->handlers[msg_id];

                if(handler.fn)
                {
                    binn obj;

                    if(binn_load(body + sizeof msg_id, &obj))
                    {
                        mdv_message request = { msg_id, &obj };
                        ret = (handler.fn(request, handler.arg) == MDV_STATUS_OK);
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

    mdv_msg_status *status = mdv_unbinn_status(msg.body);
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

    binn *msg = mdv_binn_hello(&hello);
    if (!msg)
        return false;

    if (!mdv_client_send(client, mdv_msg_hello_id, msg))
    {
        binn_free(msg);
        return false;
    }

    binn_free(msg);

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
    return client->message;
}
