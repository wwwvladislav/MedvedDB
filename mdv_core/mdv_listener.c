#include "mdv_listener.h"
#include <nng/protocol/reqrep0/rep.h>
#include <mdv_log.h>


bool mdv_listener_init(mdv_listener *lstnr, mdv_config const *cfg)
{
    int rv = nng_rep0_open_raw(&lstnr->sock);
    if (rv != 0)
    {
        MDV_LOGE("NNG socket creation failed");
        return false;
    }

    return true;
}


void mdv_listener_free(mdv_listener *lstnr)
{}
