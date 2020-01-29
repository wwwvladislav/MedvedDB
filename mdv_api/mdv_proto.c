#include "mdv_proto.h"
#include <mdv_log.h>
#include <mdv_assert.h>
#include <string.h>
#include <arpa/inet.h>


const mdv_channel_t MDV_USER_CHANNEL = 0;
const mdv_channel_t MDV_PEER_CHANNEL = 1;


static const char MDV_SIGNATURE[3] = { 'M', 'D', 'V' };     /// Protocol signature


static const uint32_t MDV_VERSION = 1u;                     /// Protocol version


typedef struct
{
    char            signature[3];
    mdv_channel_t   type;
    uint32_t        version;
    mdv_uuid        uuid;
} mdv_handshake;


mdv_static_assert(sizeof(mdv_handshake) == 24);


mdv_errno mdv_handshake_write(mdv_descriptor fd, mdv_channel_t type, mdv_uuid const *uuid)
{
    mdv_handshake const handshake =
    {
        .signature = MDV_SIGNATURE,
        .type      = type,
        .version   = htonl(MDV_VERSION),
        .uuid      = *uuid
    };

    return mdv_write_all(fd, &handshake, sizeof handshake);
}


mdv_errno mdv_handshake_read(mdv_descriptor fd, mdv_channel_t *type, mdv_uuid *uuid)
{
    mdv_handshake handshake = {};

    mdv_errno err = mdv_read_all(fd, &handshake, sizeof handshake);

    if (err == MDV_OK)
    {
        if(memcmp(handshake.signature, MDV_SIGNATURE, sizeof MDV_SIGNATURE) != 0)
        {
            MDV_LOGE("Invalid protocol signature");
            return MDV_FAILED;
        }

        handshake.version = ntohl(handshake.version);

        if(handshake.version != MDV_VERSION)
        {
            MDV_LOGE("Invalid protocol version");
            return MDV_FAILED;
        }

        *uuid = handshake.uuid;
    }

    return err;
}
