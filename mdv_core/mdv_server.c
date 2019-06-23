#include "mdv_server.h"
#include "mdv_config.h"
#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <string.h>
#include <mdv_version.h>
#include <arpa/inet.h>


typedef struct mdv_server_work
{
    enum
    {
        MDV_SERVER_WORK_INIT,
        MDV_SERVER_WORK_RECV,
        MDV_SERVER_WORK_SEND
    }                           state;
    nng_socket                  sock;
    nng_aio                    *aio;
    nng_msg                    *msg;
    mdv_message_handler        *handlers;
} mdv_server_work;


struct mdv_server
{
    nng_socket      sock;
    nng_listener    listener;

    struct
    {
        uint32_t         count;
        mdv_server_work *list;
    } works;

    mdv_message_handler handlers[mdv_msg_count];
};


static mdv_message mdv_server_hello_handler(mdv_message msg, void *arg)
{
    (void)arg;

    if (msg.id != mdv_msg_hello_id)
    {
        MDV_LOGE("Invalid handler was registered for '%s' message", mdv_msg_name(msg.id));
        return mdv_no_message;
    }

    mdv_msg_hello hello = {};

    if (!mdv_unbinn_hello(msg.body, &hello))
    {
        MDV_LOGE("Invalid '%s' message", mdv_msg_name(msg.id));
        return mdv_no_message;
    }

    static uint8_t const signature[] = MDV_HELLO_SIGNATURE;

    if (memcmp(signature, hello.signature, sizeof signature) != 0)
    {
        MDV_LOGE("Signature for '%s' message is incorrect", mdv_msg_name(msg.id));
        return mdv_no_message;
    }

    if(hello.version != MDV_VERSION)
    {
        MDV_LOGE("Invalid client version");
        mdv_msg_status status = { MDV_STATUS_INVALID_VERSION, { 0 } };
        return (mdv_message) { mdv_msg_status_id, mdv_binn_status(&status) };
    }

    mdv_msg_status status = { MDV_STATUS_OK, { 0 } };
    return (mdv_message) { mdv_msg_status_id, mdv_binn_status(&status) };
}


static mdv_message mdv_server_handle_message(mdv_server_work *work, uint32_t msg_id, uint8_t *msg_body)
{
    if (msg_id <= mdv_msg_unknown_id || msg_id >= mdv_msg_count)
    {
        MDV_LOGW("Invalid message id '%u'", msg_id);
        return mdv_no_message;
    }

    mdv_message_handler handler = work->handlers[msg_id];

    if(!handler.fn)
    {
        MDV_LOGW("Handler for '%s' message was not registered.", mdv_msg_name(msg_id));
        return mdv_no_message;
    }

    binn obj;

    if(!binn_load(msg_body, &obj))
    {
        MDV_LOGW("Message '%s' discarded", mdv_msg_name(msg_id));
        return mdv_no_message;
    }

    mdv_message request = { msg_id, &obj };

    mdv_message response = handler.fn(request, handler.arg);

    return response;
}


static bool mdv_server_nng_message_build(nng_msg *nng_msg, mdv_message message)
{
    nng_msg_clear(nng_msg);

    if (message.id > mdv_msg_unknown_id
        && message.id < mdv_msg_count
        && message.body)
    {
        int err = nng_msg_append_u32(nng_msg, message.id);
        if (!err)
        {
            err = nng_msg_append(nng_msg, binn_ptr(message.body), binn_size(message.body));
            if (!err)
                return true;
            MDV_LOGE("%s (%d)", nng_strerror(err), err);
        }
        else
            MDV_LOGE("%s (%d)", nng_strerror(err), err);
    }

    return false;
}


static void mdv_server_cb(mdv_server_work *work)
{
    mdv_alloc_thread_initialize();

    switch(work->state)
    {
        case MDV_SERVER_WORK_INIT:
        {
            work->state = MDV_SERVER_WORK_RECV;
            nng_recv_aio(work->sock, work->aio);
            break;
        }

        case MDV_SERVER_WORK_RECV:
        {
            int err = nng_aio_result(work->aio);

            if(err == NNG_ECLOSED)
                break;

            if (!err)
            {
                nng_msg *msg = nng_aio_get_msg(work->aio);

                if (msg)
                {
                    size_t len = nng_msg_len(msg);
                    uint8_t *body = (uint8_t *)nng_msg_body(msg);

                    if (len >= sizeof(uint32_t) && body)
                    {
                        uint32_t msg_id = ntohl(*(uint32_t*)body);
                        MDV_LOGI("<<< '%s' %zu bytes", mdv_msg_name(msg_id), len - sizeof msg_id);

                        mdv_message response = mdv_server_handle_message(work, msg_id, body + sizeof msg_id);

                        if (mdv_server_nng_message_build(msg, response))
                        {
                            work->state = MDV_SERVER_WORK_SEND;
                            binn_free(response.body);
                            work->msg = msg;
                            nng_aio_set_msg(work->aio, msg);
                            nng_send_aio(work->sock, work->aio);
                            break;
                        }
                        else
                            MDV_LOGW("Message '%s' discarded", mdv_msg_name(msg_id));

                        binn_free(response.body);
                    }
                    else
                        MDV_LOGW("Invalid message discarded. Size is %zu bytes", len);

                    nng_msg_free(msg);
                }
            }
            else
                MDV_LOGE("%s (%d)", nng_strerror(err), err);

            nng_recv_aio(work->sock, work->aio);

            break;
        }

        case MDV_SERVER_WORK_SEND:
        {
            int err = nng_aio_result(work->aio);

            if(err == NNG_ECLOSED)
                break;

            if (err)
            {
                MDV_LOGE("%s (%d)", nng_strerror(err), err);
                nng_msg_free(work->msg);
            }

            work->msg = 0;

            nng_recv_aio(work->sock, work->aio);

            work->state = MDV_SERVER_WORK_RECV;

            break;
        }
    }
}


static bool mdv_server_works_create(mdv_server *srvr)
{
    srvr->works.count = MDV_CONFIG.server.workers;

    srvr->works.list = mdv_alloc(srvr->works.count * sizeof(mdv_server_work));

    if (!srvr->works.list)
    {
        MDV_LOGE("No memory for server workers");
        return false;
    }

    memset(srvr->works.list, 0, srvr->works.count * sizeof(mdv_server_work));

    for(uint32_t i = 0; i < srvr->works.count; ++i)
    {
        srvr->works.list[i].sock = srvr->sock;
        srvr->works.list[i].state = MDV_SERVER_WORK_INIT;
        srvr->works.list[i].msg = 0;
        srvr->works.list[i].handlers = srvr->handlers;

        int rv = nng_aio_alloc(&srvr->works.list[i].aio, (void (*)(void *))mdv_server_cb, &srvr->works.list[i]);

        if (rv != 0)
        {
            MDV_LOGE("nng_aio_alloc failed: %d", rv);
            return false;
	}
    }

    return true;
}


static void mdv_server_works_init(mdv_server *srvr)
{
    for(uint32_t i = 0; i < srvr->works.count; ++i)
        mdv_server_cb(srvr->works.list + i);
}


mdv_server * mdv_server_create()
{
    mdv_server *server = mdv_alloc(sizeof(mdv_server));
    if(!server)
    {
        MDV_LOGE("Memory allocation for server was failed");
        return 0;
    }

    memset(server->handlers, 0, sizeof server->handlers);

    int err = nng_rep0_open_raw(&server->sock);
    if (err)
    {
        MDV_LOGE("NNG socket creation failed: %s (%d)", nng_strerror(err), err);
        mdv_server_free(server);
        return 0;
    }

    if (!mdv_server_works_create(server))
    {
        mdv_server_free(server);
        return 0;
    }

    err = nng_listener_create(&server->listener, server->sock, MDV_CONFIG.server.listen.ptr);
    if (err)
    {
        MDV_LOGE("NNG listener not created: %s (%d)", nng_strerror(err), err);
        mdv_server_free(server);
        return 0;
    }

    mdv_server_handler_reg(server, mdv_msg_hello_id, (mdv_message_handler){ 0, mdv_server_hello_handler });

    return server;
}


bool mdv_server_start(mdv_server *srvr)
{
    int err = nng_listener_start(srvr->listener, 0);

    if (!err)
    {
        mdv_server_works_init(srvr);
        return true;
    }

    MDV_LOGE("NNG listener not started: %s (%d)", nng_strerror(err), err);

    return false;
}


void mdv_server_free(mdv_server *srvr)
{
    nng_listener_close(srvr->listener);

    nng_close(srvr->sock);

    for(uint32_t i = 0; i < srvr->works.count; ++i)
    {
        nng_aio_stop(srvr->works.list[i].aio);
        nng_aio_free(srvr->works.list[i].aio);
        nng_msg_free(srvr->works.list[i].msg);
    }

    mdv_free(srvr->works.list);

    mdv_free(srvr);
}


bool mdv_server_handler_reg(mdv_server *srvr, uint32_t msg_id, mdv_message_handler handler)
{
    if (msg_id >= sizeof srvr->handlers / sizeof *srvr->handlers)
    {
        MDV_LOGE("Server message handler not registered. Message ID %u is invalid.", msg_id);
        return false;
    }

    srvr->handlers[msg_id] = handler;

    return true;
}

