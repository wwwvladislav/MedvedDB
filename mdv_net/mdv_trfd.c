#include "mdv_trfd.h"
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <stdatomic.h>


typedef struct
{
    mdv_transport       base;
    atomic_int_fast32_t rc;
    mdv_descriptor      fd;
} mdv_trfd;


static mdv_transport * mdv_trfd_retain(mdv_transport *tr)
{
    mdv_trfd *trfd = (mdv_trfd *)tr;
    atomic_fetch_add_explicit(&trfd->rc, 1, memory_order_acquire);
    return tr;
}


static void mdv_trfd_free(mdv_trfd *trfd)
{
    mdv_descriptor_close(trfd->fd);
    mdv_free(trfd);
}


static uint32_t mdv_trfd_release(mdv_transport *tr)
{
    if(!tr)
        return 0;

    mdv_trfd *trfd = (mdv_trfd *)tr;

    uint32_t rc = atomic_fetch_sub_explicit(&trfd->rc, 1, memory_order_release) - 1;

    if (!rc)
        mdv_trfd_free(trfd);

    return rc;
}


static mdv_errno mdv_trfd_write(mdv_transport *tr, void const *data, size_t *len)
{
    mdv_trfd *trfd = (mdv_trfd *)tr;
    return mdv_write(trfd->fd, data, len);
}


static mdv_errno mdv_trfd_write_all(mdv_transport *tr, void const *data, size_t len)
{
    mdv_trfd *trfd = (mdv_trfd *)tr;
    return mdv_write_all(trfd->fd, data, len);
}


static mdv_errno mdv_trfd_read(mdv_transport *tr, void *data, size_t *len)
{
    mdv_trfd *trfd = (mdv_trfd *)tr;
    return mdv_read(trfd->fd, data, len);
}


static mdv_errno mdv_trfd_read_all(mdv_transport *tr, void *data, size_t len)
{
    mdv_trfd *trfd = (mdv_trfd *)tr;
    return mdv_read_all(trfd->fd, data, len);
}


static mdv_errno mdv_trfd_skip(mdv_transport *tr, size_t len)
{
    mdv_trfd *trfd = (mdv_trfd *)tr;
    return mdv_skip(trfd->fd, len);
}


static mdv_descriptor mdv_trfd_fd(mdv_transport *tr)
{
    mdv_trfd *trfd = (mdv_trfd *)tr;
    return trfd->fd;
}


mdv_transport * mdv_trfd_create(mdv_descriptor fd)
{
    mdv_trfd *trfd = mdv_alloc(sizeof(mdv_trfd));

    if (!trfd)
    {
        MDV_LOGE("No memory for file descriptor based transport");
        return 0;
    }

    static const mdv_itransport vtbl =
    {
        .retain     = mdv_trfd_retain,
        .release    = mdv_trfd_release,
        .write      = mdv_trfd_write,
        .write_all  = mdv_trfd_write_all,
        .read       = mdv_trfd_read,
        .read_all   = mdv_trfd_read_all,
        .skip       = mdv_trfd_skip,
        .fd         = mdv_trfd_fd,
    };

    trfd->base.vptr = &vtbl;

    atomic_init(&trfd->rc, 1);

    trfd->fd = fd;

    return &trfd->base;
}
