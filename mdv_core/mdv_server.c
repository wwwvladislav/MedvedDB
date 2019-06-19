#include "mdv_server.h"
#include <nng/protocol/reqrep0/rep.h>
#include <mdv_log.h>
#include <mdv_alloc.h>
#include <string.h>
#include "mdv_config.h"


static void mdv_server_cb(mdv_server_work *work)
{

}


static bool mdv_server_works_init(mdv_server *srvr)
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

        int rv = nng_aio_alloc(&srvr->works.list[i].aio, (void (*)(void *))mdv_server_cb, &srvr->works.list[i]);

        if (rv != 0)
        {
            MDV_LOGE("nng_aio_alloc failed: %d", rv);
            return false;
	}
    }

    return true;
}


bool mdv_server_init(mdv_server *srvr)
{
    int rv = nng_rep0_open_raw(&srvr->sock);
    if (rv != 0)
    {
        MDV_LOGE("NNG socket creation failed");
        return false;
    }

    if (!mdv_server_works_init(srvr))
        return false;

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
