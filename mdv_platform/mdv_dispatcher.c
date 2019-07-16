#include "mdv_dispatcher.h"
#include "mdv_condvar.h"
#include "mdv_mutex.h"
#include "mdv_alloc.h"
#include "mdv_log.h"
#include "mdv_hashmap.h"
#include "mdv_rollbacker.h"
#include "mdv_stack.h"
#include <string.h>


enum
{
    MDV_DISP_REQS = 16      ///< Number of simultaneously sent requests
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
    mdv_descriptor      fd;             ///< File descriptor
    mdv_mutex          *fd_mutex;       ///< Mutex for requests fd guard
    mdv_msg             message;        ///< Current message
    mdv_hashmap         handlers;       ///< Message handlers (id -> mdv_msg_handler)
    mdv_hashmap         requests;       ///< Requests map (request_id -> mdv_request)
    mdv_mutex          *requests_mutex; ///< Mutex for requests map guard
    mdv_condvars_pool   cvs;            ///< conditinal variables pool
};


static size_t mdv_id_hash(void const *id)               { return *(uint16_t*)id; }

static int mdv_id_cmp(void const *id1, void const *id2) { return (int)*(uint16_t*)id1 - *(uint16_t*)id2; }


mdv_dispatcher * mdv_dispatcher_create(mdv_descriptor fd, size_t size, mdv_dispatcher_handler const *handlers)
{
    mdv_rollbacker(5) rollbacker;
    mdv_rollbacker_clear(rollbacker);

    mdv_dispatcher *pd = (mdv_dispatcher *)mdv_alloc(sizeof(mdv_dispatcher));

    if (!pd)
    {
        MDV_LOGE("No memory for messages dispatcher");
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, pd);

    memset(&pd->message, 0, sizeof pd->message);


    pd->fd = fd;
    pd->fd_mutex = mdv_mutex_create();

    if (!pd->fd_mutex)
    {
        MDV_LOGE("Mutex for messages dispatcher not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, pd->fd_mutex);


    pd->requests_mutex = mdv_mutex_create();

    if (!pd->requests_mutex)
    {
        MDV_LOGE("Mutex for messages dispatcher not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, pd->requests_mutex);


    if (!mdv_hashmap_init(pd->handlers, mdv_dispatcher_handler, id, size, &mdv_id_hash, &mdv_id_cmp))
    {
        MDV_LOGE("Handlers map for messages dispatcher not created");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, _mdv_hashmap_free, &pd->handlers);

    for(size_t i = 0; i < size; ++i)
    {
        if (!mdv_hashmap_insert(pd->handlers, handlers[i]))
        {
            MDV_LOGE("No memoyr for message handler");
            mdv_rollback(rollbacker);
            return 0;
        }
    }


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
        mdv_condvar *cv = mdv_condvar_create();

        if (!cv)
        {
            MDV_LOGE("Conditional variable for messages dispatcher not created");
            mdv_stack_foreach(pd->cvs, mdv_condvar *, entry)
                mdv_condvar_free(*entry);
            mdv_rollback(rollbacker);
            return 0;
        }

        mdv_stack_push(pd->cvs, cv);
    }

    return pd;
}


void mdv_dispatcher_free(mdv_dispatcher *pd)
{
    if (pd)
    {
        mdv_stack_foreach(pd->cvs, mdv_condvar *, entry)
            mdv_condvar_free(*entry);
        mdv_hashmap_foreach(pd->requests, mdv_request, entry)
            mdv_condvar_free(entry->data.cv);
        mdv_hashmap_free(pd->handlers);
        mdv_hashmap_free(pd->requests);
        mdv_mutex_free(pd->fd_mutex);
        mdv_mutex_free(pd->requests_mutex);
        mdv_free(pd);
    }
}


mdv_errno mdv_dispatcher_send(mdv_dispatcher *pd, mdv_msg const *req, mdv_msg *resp, size_t timeout)
{
    mdv_errno err = MDV_OK;

    // Register request
    mdv_list_entry(mdv_request) *entry = 0;

    if (mdv_mutex_lock(pd->requests_mutex) == MDV_OK)
    {
        mdv_condvar **pcv = mdv_stack_pop(pd->cvs);

        if (pcv)
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
                mdv_stack_push(pd->cvs, mreq.cv);
                mdv_hashmap_erase(pd->requests, mreq.request_id);
            }
        }
        else
            err = MDV_BUSY;

        mdv_mutex_unlock(pd->requests_mutex);
    }

    // Send request
    if (err == MDV_OK)
    {
        err = mdv_dispatcher_post(pd, req);

        if (err != MDV_OK)
        {
            MDV_LOGE("Request sending failed");

            if (mdv_mutex_lock(pd->requests_mutex) == MDV_OK)
            {
                mdv_stack_push(pd->cvs, entry->data.cv);
                mdv_hashmap_erase(pd->requests, entry->data.request_id);
                mdv_mutex_unlock(pd->requests_mutex);
            }
        }
    }

    // Wait response
    if (err == MDV_OK)
    {
        while(!entry->data.is_ready)
        {
            err = mdv_condvar_timedwait(entry->data.cv, timeout);

            if (!entry->data.is_ready
                    && err == MDV_OK)
                continue;
        }

        if (mdv_mutex_lock(pd->requests_mutex) == MDV_OK)
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

            mdv_stack_push(pd->cvs, entry->data.cv);
            mdv_hashmap_erase(pd->requests, entry->data.request_id);

            mdv_mutex_unlock(pd->requests_mutex);
        }
    }

    return err;
}


mdv_errno mdv_dispatcher_post(mdv_dispatcher *pd, mdv_msg const *msg)
{
    mdv_errno err = MDV_FAILED;

    if (mdv_mutex_lock(pd->fd_mutex) == MDV_OK)
    {
        err = mdv_write_msg(pd->fd, msg);

        if (err != MDV_OK)
            MDV_LOGE("Request posting failed");

        mdv_mutex_unlock(pd->fd_mutex);
    }

    return err;
}


mdv_errno mdv_dispatcher_read(mdv_dispatcher *pd)
{
    mdv_errno err = mdv_read_msg(pd->fd, &pd->message);

    if (err != MDV_OK)
        return err;

    int msg_is_handled = 0;

    if (mdv_mutex_lock(pd->requests_mutex) == MDV_OK)
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
                mdv_stack_push(pd->cvs, req->cv);
                mdv_hashmap_erase(pd->requests, req->request_id);
            }
        }

        mdv_mutex_unlock(pd->requests_mutex);
    }

    // Message not handled
    if (!msg_is_handled)
    {
        mdv_dispatcher_handler *handler = mdv_hashmap_find(pd->handlers, pd->message.hdr.id);

        if (handler)
            err = handler->fn(&pd->message, handler->arg);
        else
            MDV_LOGW("Message is discarded due to appropriate handler not found");

        mdv_free_msg(&pd->message);
    }

    return err;
}
