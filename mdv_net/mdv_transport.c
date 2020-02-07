#include "mdv_transport.h"


mdv_transport * mdv_transport_retain    (mdv_transport *tr)                                 { return tr->vptr->retain(tr); }
uint32_t        mdv_transport_release   (mdv_transport *tr)                                 { return tr ? tr->vptr->release(tr) : 0; }
mdv_errno       mdv_transport_write     (mdv_transport *tr, void const *data, size_t *len)  { return tr->vptr->write(tr, data, len); }
mdv_errno       mdv_transport_write_all (mdv_transport *tr, void const *data, size_t len)   { return tr->vptr->write_all(tr, data, len); }
mdv_errno       mdv_transport_read      (mdv_transport *tr, void *data, size_t *len)        { return tr->vptr->read(tr, data, len); }
mdv_errno       mdv_transport_read_all  (mdv_transport *tr, void *data, size_t len)         { return tr->vptr->read_all(tr, data, len); }
mdv_errno       mdv_transport_skip      (mdv_transport *tr, size_t len)                     { return tr->vptr->skip(tr, len); }
mdv_descriptor  mdv_transport_fd        (mdv_transport *tr)                                 { return tr->vptr->fd(tr); }
