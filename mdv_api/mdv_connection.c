#include "mdv_connection.h"
#include "mdv_messages.h"
#include "mdv_version.h"
#include <mdv_alloc.h>
#include <mdv_binn.h>
#include <stdbool.h>
#include <nng/nng.h>
#include <nng/protocol/reqrep0/req.h>
#include <stdio.h>


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


struct s_mdv_connection
{
    nng_socket sock;
    nng_dialer dialer;
};


mdv_connection * mdv_connect(char const *addr)
{
    allocator_init();

    mdv_connection *connection = mdv_alloc(sizeof(mdv_connection));

    int err = nng_req0_open(&connection->sock);
    if (err)
    {
        printf("%s", nng_strerror(err));
        mdv_disconnect(connection);
        return 0;
    }

    err = nng_dial(connection->sock, addr, &connection->dialer, 0);
    if (err)
    {
        printf("%s", nng_strerror(err));
        mdv_disconnect(connection);
        return 0;
    }

    mdv_msg_hello hello =
    {
        MDV_HELLO_SIGNATURE,
        MDV_VERSION
    };

    binn *msg = mdv_binn_hello(&hello);
    if (!msg)
    {
        mdv_disconnect(connection);
        return 0;
    }

    if (!mdv_send_msg(connection, mdv_msg_hello_id, msg))
    {
        mdv_disconnect(connection);
        return 0;
    }

    return connection;
}


void mdv_disconnect(mdv_connection *connection)
{
    if (!connection)
        return;
    nng_dialer_close(connection->dialer);
    nng_close(connection->sock);
    mdv_free(connection);
    allocator_finalize();
}


bool mdv_send_msg(mdv_connection *connection, uint32_t id, binn *obj)
{
    nng_msg * msg;
    int err = nng_msg_alloc(&msg, 0);
    if (err)
    {
        printf("%s", nng_strerror(err));
        return false;
    }

    err = nng_msg_append_u32(msg, id);
    if (err)
    {
        printf("%s", nng_strerror(err));
        nng_msg_free(msg);
        return false;
    }

    err = nng_msg_append(msg, binn_ptr(obj), binn_size(obj));
    if (err)
    {
        printf("%s", nng_strerror(err));
        nng_msg_free(msg);
        return false;
    }

    err = nng_sendmsg(connection->sock, msg, 0);
    if (err)
    {
        printf("%s", nng_strerror(err));
        nng_msg_free(msg);
        return false;
    }

    return true;
}
