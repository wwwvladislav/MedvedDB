#include "mdv_peer.h"
#include <mdv_messages.h>
#include <mdv_version.h>
#include <mdv_protocol.h>
#include <mdv_socket.h>


mdv_errno mdv_peer_wave(mdv_peer *peer, mdv_uuid const *uuid)
{
    mdv_msg_hello const msg =
    {
        .uuid = *uuid,
        .version = MDV_VERSION
    };

    mdv_errno err;

    binn hey;

    if (!mdv_binn_hello(&msg, &hey))
        return MDV_FAILED;

    mdv_msg_hdr hdr =
    {
        .msg_id = mdv_hton16(mdv_msg_hello_id),
        .req_id = mdv_hton16(atomic_fetch_add_explicit(&peer->req_id, 1, memory_order_relaxed)),
        .body_size = mdv_hton32(binn_size(&hey))
    };

    do
    {
        err = mdv_write_all(peer->fd, &hdr, sizeof hdr);

        if (err != MDV_OK)
            break;

        err = mdv_write_all(peer->fd, binn_ptr(&hey), binn_size(&hey));
    } while(0);

    binn_free(&hey);

    return err;
}
