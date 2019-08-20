#include "mdv_dispatcher.h"
#include "mdv_condvar.h"
#include "mdv_mutex.h"
#include "mdv_alloc.h"
#include "mdv_log.h"
#include "mdv_hashmap.h"
#include "mdv_rollbacker.h"
#include "mdv_stack.h"
#include "mdv_def.h"
#include <string.h>
#include <stdatomic.h>


enum
{
    MDV_DISP_REQS = 4      ///< Number of simultaneously sent requests via single connection
};


/// Conditional variables pool
typedef mdv_stack(mdv_condvar*, MDV_DISP_REQS) mdv_condvars_pool;


/// Request state
typedef struct
{
    mdv_condvar    *cv;                 ///< Condition variable is used by client for response waiting
    mdv_msg        *resp;               ///< Response
    uint16_t        request_id;         ///< Request identifier
    uint16_t        is_ready:1;         ///< Response is ready
} mdv_request;


/// Messages dispatcher
struct mdv_dispatcher
{
    mdv_descriptor volatile fd;                         ///< File descriptor
    mdv_mutex               fd_mutex;                   ///< Mutex for fd guard
    mdv_msg                 message;                    ///< Current message
    mdv_hashmap             handlers;                   ///< Message handlers (id -> mdv_msg_handler)
    mdv_hashmap             requests;                   ///< Requests map (request_id -> mdv_request)
    mdv_mutex               requests_mutex;             ///< Mutex for requests map guard
    mdv_condvars_pool       cvs;                        ///< unused conditinal variables pointers
    atomic_ushort           id;                         ///< id generator
    mdv_condvar             condvars[MDV_DISP_REQS];    ///< conditinal variables pool
};


static size_t mdv_id_hash(void const *id)               { return *(uint16_t*)id; }

static int mdv_id_cmp(void const *id1, void const *id2) { return (int)*(uint16_t*)id1 - *(uint16_t*)id2; }


mdv_dispatcher * mdv_dispatcher_create(mdv_descriptor fd)
{
    mdv_rollbacker(5) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_dispatcher *pd = (mdv_dispatcher *)mdv_alloc(sizeof(mdv_dispatcher), "dispatcher");

    if (!pd)
    {
        MDV_LOGE("No memory for messages dispatcher");
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, pd, "dispatcher");

    memset(&pd->message, 0, sizeof pd->message);

    atomic_init(&pd->id, 0);


    pd->fd = fd;

    if (mdv_mutex_create(&pd->fd_mutex) != MDV_OK)
    {
        MDV_LOGE("Mutex for messages dispatcher not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &pd->fd_mutex);


    if (mdv_mutex_create(&pd->requests_mutex) != MDV_OK)
    {
        MDV_LOGE("Mutex for messages dispatcher not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &pd->requests_mutex);


    if (!mdv_hashmap_init(pd->handlers, mdv_dispatcher_handler, id, 4, &mdv_id_hash, &mdv_id_cmp))
    {
        MDV_LOGE("Handlers map for messages dispatcher not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &pd->handlers);


    if (!mdv_hashmap_init(pd->requests, mdv_request, request_id, MDV_DISP_REQS, &mdv_id_hash, &mdv_id_cmp))
    {
        MDV_LOGE("Requests map for messages dispatcher not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &pd->requests);


    mdv_stack_clear(pd->cvs);

    for (size_t i = 0; i < MDV_DISP_REQS; ++i)
    {
        if (mdv_condvar_create(pd->condvars + i) != MDV_OK
            || !mdv_stack_push(pd->cvs, &pd->condvars[i]))
        {
            MDV_LOGE("Conditional variable for messages dispatcher not created");
            for(size_t j = 0; j < i; ++j)
                mdv_condvar_free(pd->condvars + j);
            mdv_rollback(rollbacker);
            return 0;
        }
    }

    MDV_LOGD("Messages dispatcher %p created", pd);

    return pd;
}


void mdv_dispatcher_free(mdv_dispatcher *pd)
{
    if (pd)
    {
        MDV_LOGD("Messages dispatcher %p deleted", pd);

        if (mdv_mutex_lock(&pd->requests_mutex) == MDV_OK)
        {
            mdv_hashmap_foreach(pd->requests, mdv_request, entry)
                mdv_condvar_signal(entry->cv);
            mdv_mutex_unlock(&pd->requests_mutex);
        }

        for (size_t i = 0; i < sizeof pd->condvars / sizeof *pd->condvars; ++i)
            mdv_condvar_free(pd->condvars + i);
        mdv_hashmap_free(pd->handlers);
        mdv_hashmap_free(pd->requests);
        mdv_mutex_free(&pd->fd_mutex);
        mdv_mutex_free(&pd->requests_mutex);
        memset(pd, 0, sizeof *pd);
        mdv_free(pd, "dispatcher");
    }
}


mdv_errno mdv_dispatcher_reg(mdv_dispatcher *pd, mdv_dispatcher_handler const *handler)
{
    if (!mdv_hashmap_insert(pd->handlers, *handler))
    {
        MDV_LOGE("No memoyry for message handler");
        return MDV_NO_MEM;
    }
    return MDV_OK;
}


void mdv_dispatcher_set_fd(mdv_dispatcher *pd, mdv_descriptor fd)
{
    pd->fd = fd;
}


void mdv_dispatcher_close_fd(mdv_dispatcher *pd)
{
    pd->fd = (void*)MDV_INVALID_DESCRIPTOR;

    if (mdv_mutex_lock(&pd->requests_mutex) == MDV_OK)
    {
        mdv_hashmap_foreach(pd->requests, mdv_request, entry)
            mdv_condvar_signal(entry->cv);
        mdv_mutex_unlock(&pd->requests_mutex);
    }
}


mdv_descriptor mdv_dispatcher_fd(mdv_dispatcher *pd)
{
    return pd->fd;
}


mdv_errno mdv_dispatcher_send(mdv_dispatcher *pd, mdv_msg *req, mdv_msg *resp, size_t timeout)
{
    mdv_errno err = MDV_OK;

    req->hdr.number = atomic_fetch_add_explicit(&pd->id, 1, memory_order_relaxed);

    // Register request
    mdv_list_entry(mdv_request) *entry = 0;

    if (mdv_mutex_lock(&pd->requests_mutex) == MDV_OK)
    {
        mdv_condvar **pcv = mdv_stack_pop(pd->cvs);

        if (*pcv)
        {
            mdv_request mreq =
            {
                .cv         = *pcv,
                .resp       = resp,
                .request_id = req->hdr.number,
                .is_ready   = 0
            };

            entry = (void*)mdv_hashmap_insert(pd->requests, mreq);

            if (!entry)
            {
                MDV_LOGE("No memory for new request");
                err = MDV_NO_MEM;
                (void)mdv_stack_push(pd->cvs, mreq.cv);
                mdv_hashmap_erase(pd->requests, mreq.request_id);
            }
        }
        else
            err = MDV_BUSY;

        mdv_mutex_unlock(&pd->requests_mutex);
    }

    // Send request
    if (err == MDV_OK)
    {
        if (mdv_mutex_lock(&pd->fd_mutex) == MDV_OK)
        {
            err = mdv_write_msg(pd->fd, req);

            if (err != MDV_OK)
                MDV_LOGE("Request sending failed");

            mdv_mutex_unlock(&pd->fd_mutex);
        }

        if (err != MDV_OK)
        {
            MDV_LOGE("Request sending failed");

            if (mdv_mutex_lock(&pd->requests_mutex) == MDV_OK)
            {
                (void)mdv_stack_push(pd->cvs, entry->data.cv);
                mdv_hashmap_erase(pd->requests, entry->data.request_id);
                mdv_mutex_unlock(&pd->requests_mutex);
            }
        }
    }

    // Wait response
    if (err == MDV_OK)
    {
        while(!entry->data.is_ready)
        {
            err = mdv_condvar_timedwait(entry->data.cv, timeout);

            if (pd->fd == MDV_INVALID_DESCRIPTOR)   // Connection closed
            {
                err = MDV_CLOSED;
                break;
            }

            if (!entry->data.is_ready
                && err == MDV_OK)
                continue;

            if (err == MDV_ETIMEDOUT)
                break;
        }

        if (mdv_mutex_lock(&pd->requests_mutex) == MDV_OK)
        {
            if (entry->data.is_ready
                    && err == MDV_OK)
            {
                *resp = *entry->data.resp;
            }
            else
            {
                if (err == MDV_OK)
                    err = MDV_ETIMEDOUT;

                if (err == MDV_ETIMEDOUT)
                    MDV_LOGE("Response timeout");
                else
                    MDV_LOGE("Response waiting failed");
            }

            (void)mdv_stack_push(pd->cvs, entry->data.cv);
            mdv_hashmap_erase(pd->requests, entry->data.request_id);

            mdv_mutex_unlock(&pd->requests_mutex);
        }
    }

    return err;
}


mdv_errno mdv_dispatcher_post(mdv_dispatcher *pd, mdv_msg *msg)
{
    msg->hdr.number = atomic_fetch_add_explicit(&pd->id, 1, memory_order_relaxed);
    return mdv_dispatcher_reply(pd, msg);
}


mdv_errno mdv_dispatcher_reply(mdv_dispatcher *pd, mdv_msg const *msg)
{
    mdv_errno err = MDV_FAILED;

    if (mdv_mutex_lock(&pd->fd_mutex) == MDV_OK)
    {
        err = mdv_write_msg(pd->fd, msg);

        if (err != MDV_OK)
            MDV_LOGE("Message posting failed");

        mdv_mutex_unlock(&pd->fd_mutex);
    }

    return err;
}


mdv_errno mdv_dispatcher_write_raw(mdv_dispatcher *pd, void const *data, size_t size)
{
    mdv_errno err = MDV_FAILED;

    if (mdv_mutex_lock(&pd->fd_mutex) == MDV_OK)
    {
        err = mdv_write_all(pd->fd, data, size);

        if (err != MDV_OK)
            MDV_LOGE("Message posting failed");

        mdv_mutex_unlock(&pd->fd_mutex);
    }

    return err;
}


mdv_errno mdv_dispatcher_read(mdv_dispatcher *pd)
{
    mdv_errno err = mdv_read_msg(pd->fd, &pd->message);

    if (err != MDV_OK)
        return err;

    int msg_is_handled = 0;

    if (mdv_mutex_lock(&pd->requests_mutex) == MDV_OK)
    {
        mdv_request *req = mdv_hashmap_find(pd->requests, pd->message.hdr.number);

        if (req)
        {
            msg_is_handled = 1;
            *req->resp = pd->message;
            req->is_ready = 1;
            memset(&pd->message, 0, sizeof pd->message);

            if (mdv_condvar_signal(req->cv) != MDV_OK)
            {
                MDV_LOGW("Response is discarded due to conditional signalization fail");
                mdv_free_msg(req->resp);
                (void)mdv_stack_push(pd->cvs, req->cv);
                mdv_hashmap_erase(pd->requests, req->request_id);
            }
        }

        mdv_mutex_unlock(&pd->requests_mutex);
    }

    // Message not handled
    if (!msg_is_handled)
    {
        mdv_dispatcher_handler *handler = mdv_hashmap_find(pd->handlers, pd->message.hdr.id);

        if (handler)
            err = handler->fn(&pd->message, handler->arg);
        else
        {
            MDV_LOGW("Message is discarded due to appropriate handler not found");
            err = MDV_NO_IMPL;
        }

        mdv_free_msg(&pd->message);
    }

    return err;
}
