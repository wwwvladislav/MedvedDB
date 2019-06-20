#include "mdv_server.h"
#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <mdv_messages.h>
#include <string.h>
#include "mdv_config.h"
#include <arpa/inet.h>


struct s_mdv_server_work
{
    enum
    {
        MDV_SERVER_WORK_INIT,
        MDV_SERVER_WORK_RECV,
        MDV_SERVER_WORK_WAIT,
        MDV_SERVER_WORK_SEND
    }           state;
    nng_socket  sock;
    nng_aio    *aio;
};


static void mdv_server_cb(mdv_server_work *work)
{
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
            if (err)
            {
                MDV_LOGE("%s", nng_strerror(err));
                break;
            }

            nng_msg *msg = nng_aio_get_msg(work->aio);
            if (msg)
            {
                size_t len = nng_msg_len(msg);
                uint8_t *body = (uint8_t *)nng_msg_body(msg);

                if (len >= sizeof(uint32_t) && body)
                {
                    uint32_t msg_id = ntohl(*(uint32_t*)body);
                    MDV_LOGI("<<< '%s' %zu bytes", mdv_msg_name(msg_id), len - sizeof msg_id);
                }

                // TODO

                nng_msg_free(msg);
		nng_recv_aio(work->sock, work->aio);
            }

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


bool mdv_server_init(mdv_server *srvr)
{
    int err = nng_rep0_open_raw(&srvr->sock);
    if (err)
    {
        MDV_LOGE("NNG socket creation failed: %s", nng_strerror(err));
        return false;
    }

    if (!mdv_server_works_create(srvr))
        return false;

    err = nng_listen(srvr->sock, MDV_CONFIG.server.listen.ptr, 0, 0);
    if (err)
    {
        MDV_LOGE("NNG listener not started: %s", nng_strerror(err));
        return false;
    }

    mdv_server_works_init(srvr);

    return true;
}


void mdv_server_free(mdv_server *srvr)
{
    nng_close(srvr->sock);

    for(uint32_t i = 0; i < srvr->works.count; ++i)
    {
        nng_aio_stop(srvr->works.list[i].aio);
        nng_aio_free(srvr->works.list[i].aio);
    }

    mdv_free(srvr->works.list);
}
