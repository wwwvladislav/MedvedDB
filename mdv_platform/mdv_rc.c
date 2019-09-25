#include "mdv_rc.h"
#include "mdv_log.h"
#include "mdv_alloc.h"
#include <stdatomic.h>


struct mdv_rc
{
    void (*destructor)(void *);
    atomic_uint_fast32_t    rc;
    uint8_t                 data[1];
};


mdv_rc * mdv_rc_create(size_t size,
                       void *arg,
                       bool (*constructor)(void *arg, void *ptr),
                       void (*destructor)(void *ptr))
{
    mdv_rc *rc = mdv_alloc(offsetof(mdv_rc, data) + size, "rc");

    if (!rc)
    {
        MDV_LOGE("No memory for reference countable object");
        return 0;
    }

    rc->destructor = destructor;

    atomic_init(&rc->rc, 1);

    if (!constructor(arg, rc->data))
    {
        MDV_LOGE("RC object constructor failed");
        mdv_free(rc, "rc");
        return 0;
    }

    return rc;
}


mdv_rc * mdv_rc_retain(mdv_rc *rc)
{
    atomic_fetch_add_explicit(&rc->rc, 1, memory_order_acquire);
    return rc;
}


void mdv_rc_release(mdv_rc *rc)
{
    if (atomic_fetch_sub_explicit(&rc->rc, 1, memory_order_release) - 1 == 0)
    {
        rc->destructor(rc->data);
        mdv_free(rc, "rc");
    }
}


void * mdv_rc_ptr(mdv_rc *rc)
{
    return rc->data;
}
