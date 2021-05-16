#include "mdv_msg.h"
#include "mdv_socket.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_limits.h>
#include <string.h>


mdv_errno mdv_write_msg(mdv_descriptor fd, mdv_msg const *msg)
{
    if (msg->hdr.size > MDV_MSG_SIZE_MAX)
    {
        MDV_LOGE("Message is too long");
        return MDV_FAILED;
    }

    mdv_msghdr const hdr =
    {
        .id     = mdv_hton16(msg->hdr.id),
        .number = mdv_hton16(msg->hdr.number),
        .size   = mdv_hton32(msg->hdr.size)
    };

    mdv_errno err = mdv_write_all(fd, &hdr, sizeof hdr);

    if (err != MDV_OK)
        return err;

    return mdv_write_all(fd, msg->payload, msg->hdr.size);
}


mdv_errno mdv_read_msg(mdv_descriptor fd, mdv_msg *msg)
{
    // Read header
    while(msg->available_size < sizeof(mdv_msghdr))
    {
        size_t len = sizeof(mdv_msghdr) - msg->available_size;

        mdv_errno err = mdv_read(fd, ((char*)&msg->hdr) + msg->available_size, &len);

        if (err != MDV_OK)
            return err;

        msg->available_size += len;

        if (msg->available_size == sizeof(mdv_msghdr))
        {
            msg->hdr.id     = mdv_ntoh16(msg->hdr.id);
            msg->hdr.number = mdv_ntoh16(msg->hdr.number);
            msg->hdr.size   = mdv_ntoh32(msg->hdr.size);

            if (msg->hdr.size > MDV_MSG_SIZE_MAX)
            {
                MDV_LOGE("Incoming message is too long");
                memset(msg, 0, sizeof *msg);
                return MDV_FAILED;
            }


            if (msg->hdr.size)
            {
                msg->payload = mdv_alloc(msg->hdr.size);

                if (!msg->payload)
                {
                    MDV_LOGE("No memory for incoming message");
                    memset(msg, 0, sizeof *msg);
                    return MDV_NO_MEM;
                }
            }
            else
                msg->payload = 0;

            break;
        }
    }

    while(msg->available_size - sizeof(mdv_msghdr) < msg->hdr.size)
    {
        uint32_t const available_size = msg->available_size - sizeof(mdv_msghdr);

        size_t len = msg->hdr.size - available_size;

        mdv_errno err = mdv_read(fd, (char*)msg->payload + available_size, &len);

        if (err != MDV_OK)
            return err;

        msg->available_size += len;
    }

    return MDV_OK;
}


void mdv_free_msg(mdv_msg *msg)
{
    mdv_free(msg->payload);
    memset(msg, 0, sizeof *msg);
}
