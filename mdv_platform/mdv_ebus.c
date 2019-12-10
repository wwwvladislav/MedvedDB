#include "mdv_ebus.h"
#include "mdv_rollbacker.h"
#include "mdv_queuefd.h"
#include "mdv_vector.h"
#include "mdv_hashmap.h"
#include "mdv_alloc.h"
#include "mdv_log.h"
#include "mdv_mutex.h"
#include "mdv_algorithm.h"
#include "mdv_threads.h"


/// @cond Doxygen_Suppress

enum
{
    MDV_EBUS_QUEUE_SIZE             = 256,  ///< Event queue size
    MDV_EBUS_QUEUE_PUSH_ATTEMPTS    = 64    ///< Number of event push attempts
};


typedef struct
{
    void *arg;                              ///< Argument passed to event handler
    mdv_event_handler handler;              ///< Event handler
} mdv_ebus_subscriber;


typedef struct
{
    mdv_event_type          type;           ///< Event type
    mdv_vector             *subscribers;    ///< vector<mdv_ebus_subscriber>
} mdv_event_handlers;


typedef mdv_queuefd(mdv_event*, MDV_EBUS_QUEUE_SIZE) mdv_evt_queue;


struct mdv_ebus
{
    atomic_uint_fast32_t    rc;             ///< Event bus references counter
    uint32_t                events_count;   ///< Event types count
    mdv_mutex               mutex;          ///< Mutex for event handlers guard
    mdv_hashmap            *handlers;       ///< Event handlers (hashmap<mdv_event_handlers>)
    mdv_threadpool         *threads;        ///< Thread pool
    atomic_size_t           idx;            ///< Counter is used for queue selection during the event publishing
    size_t                  size;           ///< Event queues count
    mdv_evt_queue          *queues;         ///< Event queues
    atomic_uint_fast32_t   *events_gen;     ///< Counters for each generated event type
};


typedef struct mdv_ebus_context
{
    mdv_ebus      *ebus;
    mdv_evt_queue *events;
} mdv_ebus_context;


typedef mdv_threadpool_task(mdv_ebus_context) mdv_ebus_task;


/// @endcond


static bool mdv_ebus_subscriber_equ(void const *a, void const *b)
{
    mdv_ebus_subscriber const *sa = a, *sb = b;
    return sa->arg == sb->arg
            &&  sa->handler == sb->handler;
}


static mdv_vector * mdv_ebus_evt_subscribers(mdv_ebus *ebus, mdv_event_type type)
{
    mdv_vector *subscribers = 0;

    if (mdv_mutex_lock(&ebus->mutex) == MDV_OK)
    {
        mdv_event_handlers *handlers = mdv_hashmap_find(ebus->handlers, &type);

        if (handlers)
            subscribers = mdv_vector_retain(handlers->subscribers);

        mdv_mutex_unlock(&ebus->mutex);
    }

    return subscribers;
}


static mdv_errno mdv_ebus_event_process(mdv_ebus *ebus, mdv_event *event)
{
    mdv_errno err = MDV_NO_IMPL;

    mdv_vector *subscribers = mdv_ebus_evt_subscribers(ebus, event->type);

    if (subscribers)
    {
        mdv_vector_foreach(subscribers, mdv_ebus_subscriber, sbr)
        {
            mdv_errno res = sbr->handler(sbr->arg, event);

            if (err == MDV_OK || err == MDV_NO_IMPL)
                err = res;
        }
        mdv_vector_release(subscribers);
    }

    return err;
}


static void mdv_ebus_evt_handler(uint32_t events, mdv_threadpool_task_base *task_base)
{
    mdv_ebus_task *task = (mdv_ebus_task*)task_base;
    mdv_ebus_context *context = &task->context;
    mdv_ebus *ebus = context->ebus;

    if (events & MDV_EPOLLIN)
    {
        mdv_event *event = 0;

        if (mdv_queuefd_pop(*context->events, event))
        {
            atomic_fetch_sub_explicit(ebus->events_gen + event->type, 1, memory_order_relaxed);

            mdv_ebus_event_process(ebus, event);

            event->vptr->release(event);
        }
    }
}


static size_t mdv_u32_hash(uint32_t const *v)
{
    return *v;
}


static int mdv_u32_keys_cmp(uint32_t const *a, uint32_t const *b)
{
    return (int)*a - *b;
}


mdv_event * mdv_event_create(mdv_event_type type, size_t size)
{
    mdv_event *event = mdv_alloc(size, "event");

    if (!event)
    {
        MDV_LOGE("No memory for new event.");
        return 0;
    }

    static mdv_ievent vtbl =
    {
        .retain = mdv_event_retain,
        .release = mdv_event_release
    };

    event->vptr = &vtbl;
    event->type = type;
    atomic_init(&event->rc, 1);

    return event;
}


mdv_event * mdv_event_retain(mdv_event *event)
{
    atomic_fetch_add_explicit(&event->rc, 1, memory_order_acquire);
    return event;
}


uint32_t mdv_event_release(mdv_event *event)
{
    uint32_t rc = 0;

    if (event)
    {
        rc = atomic_fetch_sub_explicit(&event->rc, 1, memory_order_release) - 1;
        if (!rc)
            mdv_free(event, "event");
    }

    return rc;
}


mdv_ebus * mdv_ebus_create(mdv_ebus_config const *config)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    mdv_ebus *ebus = mdv_alloc(sizeof(mdv_ebus)
                                + sizeof(mdv_evt_queue) * config->event.queues_count
                                + sizeof(atomic_uint_fast32_t) * (1 + config->event.max_id),
                                "ebus");

    if (!ebus)
    {
        MDV_LOGE("No memory for events bus");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, ebus, "ebus");

    atomic_init(&ebus->rc, 1);
    atomic_init(&ebus->idx, 0);

    ebus->events_count = (uint32_t)config->event.max_id + 1;

    ebus->size = config->event.queues_count
                    ? config->event.queues_count
                    : 1;

    ebus->queues = (void*)(ebus + 1);

    ebus->events_gen = (void*)(ebus->queues + config->event.queues_count);

    for(uint32_t i = 0; i < ebus->events_count; ++i)
        atomic_init(ebus->events_gen + i, 0);

    if(mdv_mutex_create(&ebus->mutex) != MDV_OK)
    {
        MDV_LOGE("Miutex creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &ebus->mutex);

    ebus->handlers = mdv_hashmap_create(mdv_event_handlers,
                                        type,
                                        ebus->size,
                                        mdv_u32_hash,
                                        mdv_u32_keys_cmp);

    if (!ebus->handlers)
    {
        MDV_LOGE("No memory for events bus handlers");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, ebus->handlers);

    ebus->threads = mdv_threadpool_create(&config->threadpool);

    if (!ebus->threads)
    {
        MDV_LOGE("Threadpool creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_threadpool_free, ebus->threads);

    for(size_t i = 0; i < ebus->size; ++i)
    {
        mdv_queuefd_init(ebus->queues[i]);

        if (!mdv_queuefd_ok(ebus->queues[i]))
        {
            mdv_threadpool_stop(ebus->threads);

            for(size_t j = 0; j < i; ++j)
                mdv_queuefd_free(ebus->queues[j]);

            MDV_LOGE("Events queue creation failed");

            mdv_rollback(rollbacker);

            return 0;
        }

        mdv_ebus_task task =
        {
            .fd = ebus->queues[i].event,
            .fn = mdv_ebus_evt_handler,
            .context_size = sizeof(mdv_ebus_context),
            .context =
            {
                .ebus = ebus,
                .events = ebus->queues + i
            }
        };

        if (!mdv_threadpool_add(ebus->threads,
                                MDV_EPOLLEXCLUSIVE | MDV_EPOLLIN | MDV_EPOLLERR,
                                (mdv_threadpool_task_base const *)&task))
        {
            mdv_threadpool_stop(ebus->threads);

            for(size_t j = 0; j < i; ++j)
                mdv_queuefd_free(ebus->queues[j]);

            MDV_LOGE("Events queue registration failed");

            mdv_rollback(rollbacker);

            return 0;
        }
    }

    mdv_rollbacker_free(rollbacker);

    return ebus;
}


static void mdv_ebus_free(mdv_ebus *ebus)
{
    mdv_threadpool_stop(ebus->threads);
    mdv_threadpool_free(ebus->threads);

    for(size_t i = 0; i < ebus->size; ++i)
    {
        if (mdv_queuefd_size(ebus->queues[i]))
        {
            mdv_event *event = 0;

            while(mdv_queuefd_pop(ebus->queues[i], event))
            {
                event->vptr->release(event);
            }
        }
        mdv_queuefd_free(ebus->queues[i]);
    }

    mdv_hashmap_foreach(ebus->handlers, mdv_event_handlers, entry)
    {
        mdv_vector_release(entry->subscribers);
    }

    mdv_hashmap_release(ebus->handlers);

    mdv_mutex_free(&ebus->mutex);

    mdv_free(ebus, "ebus");
}


mdv_ebus * mdv_ebus_retain(mdv_ebus *ebus)
{
    atomic_fetch_add_explicit(&ebus->rc, 1, memory_order_acquire);
    return ebus;
}


uint32_t mdv_ebus_release(mdv_ebus *ebus)
{
    uint32_t rc = 0;

    if (ebus)
    {
        rc = atomic_fetch_sub_explicit(&ebus->rc, 1, memory_order_release) - 1;
        if (!rc)
            mdv_ebus_free(ebus);
    }

    return rc;
}


static mdv_event_handlers * mdv_ebus_handlers_unsafe(mdv_ebus *ebus, mdv_event_type type)
{
    mdv_event_handlers *handlers = mdv_hashmap_find(ebus->handlers, &type);

    if (!handlers)
    {
        mdv_event_handlers new_handlers =
        {
            .type = type,
            .subscribers = mdv_vector_create(4, sizeof(mdv_ebus_subscriber), &mdv_default_allocator)
        };

        if (!new_handlers.subscribers)
            return 0;
        else
        {
            handlers = mdv_hashmap_insert(ebus->handlers, &new_handlers, sizeof new_handlers);

            if (!handlers)
            {
                mdv_vector_release(new_handlers.subscribers);
                return 0;
            }
        }
    }

    return handlers;
}


static mdv_errno mdv_ebus_subscribe_unsafe(mdv_ebus *ebus,
                                           mdv_event_type type,
                                           void *arg,
                                           mdv_event_handler handler)
{
    mdv_errno err = MDV_OK;

    mdv_event_handlers *handlers = mdv_ebus_handlers_unsafe(ebus, type);

    if (handlers)
    {
        mdv_ebus_subscriber subscriber =
        {
            .arg = arg,
            .handler = handler
        };

        if (!mdv_vector_find(handlers->subscribers, &subscriber, mdv_ebus_subscriber_equ))
        {
            mdv_vector *subscribers = mdv_vector_clone(handlers->subscribers,
                                            mdv_vector_size(handlers->subscribers) + 1);

            if (subscribers)
            {
                mdv_vector_push_back(subscribers, &subscriber);
                mdv_vector_release(handlers->subscribers);
                handlers->subscribers = subscribers;
            }
            else
                err = MDV_NO_MEM;
        }
    }
    else
        err = MDV_NO_MEM;

    return err;
}


static mdv_errno mdv_ebus_unsubscribe_unsafe(mdv_ebus *ebus,
                                             mdv_event_type type,
                                             void *arg,
                                             mdv_event_handler handler)
{
    mdv_errno err = MDV_OK;

    mdv_event_handlers *handlers = mdv_hashmap_find(ebus->handlers, &type);

    if (handlers)
    {
        mdv_ebus_subscriber const subscriber =
        {
            .arg = arg,
            .handler = handler
        };

        mdv_ebus_subscriber const *registered_subscriber =
                                mdv_vector_find(handlers->subscribers,
                                                &subscriber,
                                                mdv_ebus_subscriber_equ);

        if (registered_subscriber)
        {
            mdv_vector *subscribers = mdv_vector_clone(handlers->subscribers,
                                            mdv_vector_size(handlers->subscribers));

            if (subscribers)
            {
                size_t const idx = registered_subscriber
                                    - (mdv_ebus_subscriber const *)
                                            mdv_vector_data(handlers->subscribers);
                mdv_vector_erase(subscribers, mdv_vector_at(subscribers, idx));

                mdv_swap(mdv_vector *, handlers->subscribers, subscribers);

                // Waiting for handlers completion
                while(mdv_vector_refs(subscribers) > 1)
                    mdv_sleep(10);

                mdv_vector_release(subscribers);
            }
            else
                err = MDV_NO_MEM;
        }
    }

    return err;
}


mdv_errno mdv_ebus_subscribe(mdv_ebus *ebus,
                             mdv_event_type type,
                             void *arg,
                             mdv_event_handler handler)
{
    mdv_errno err = mdv_mutex_lock(&ebus->mutex);

    if (err == MDV_OK)
    {
        err = mdv_ebus_subscribe_unsafe(ebus, type, arg, handler);
        mdv_mutex_unlock(&ebus->mutex);
    }

    if (err != MDV_OK)
        MDV_LOGE("Event subscriber registration failed with error %d", err);

    return err;
}


mdv_errno mdv_ebus_subscribe_all(mdv_ebus *ebus,
                                 void *arg,
                                 mdv_event_handler_type const *handlers,
                                 size_t count)
{
    // TODO: Fix deadlock. The event might be fired during the subscription.

    mdv_errno err = mdv_mutex_lock(&ebus->mutex);

    if (err == MDV_OK)
    {
        for(size_t i = 0; i < count; ++i)
        {
            err = mdv_ebus_subscribe_unsafe(ebus,
                                            handlers[i].type,
                                            arg,
                                            handlers[i].handler);

            if (err != MDV_OK)
            {
                for(; i > 0; --i)
                    mdv_ebus_unsubscribe_unsafe(ebus,
                                                handlers[i - 1].type,
                                                arg,
                                                handlers[i - 1].handler);

                break;
            }
        }

        mdv_mutex_unlock(&ebus->mutex);
    }

    if (err != MDV_OK)
        MDV_LOGE("Event subscriber registration failed with error %d", err);

    return err;
}


void mdv_ebus_unsubscribe(mdv_ebus *ebus,
                          mdv_event_type type,
                          void *arg,
                          mdv_event_handler handler)
{
    mdv_errno err = mdv_mutex_lock(&ebus->mutex);

    if (err == MDV_OK)
    {
        err = mdv_ebus_unsubscribe_unsafe(ebus, type, arg, handler);
        mdv_mutex_unlock(&ebus->mutex);
    }

    if (err != MDV_OK)
        MDV_LOGE("Event subscriber unregistration failed with error %d", err);
}


void mdv_ebus_unsubscribe_all(mdv_ebus *ebus,
                              void *arg,
                              mdv_event_handler_type const *handlers,
                              size_t count)
{
    mdv_errno err = mdv_mutex_lock(&ebus->mutex);

    if (err == MDV_OK)
    {
        for(size_t i = 0; i < count; ++i)
            mdv_ebus_unsubscribe_unsafe(ebus,
                                        handlers[i].type,
                                        arg,
                                        handlers[i].handler);

        mdv_mutex_unlock(&ebus->mutex);
    }

    if (err != MDV_OK)
        MDV_LOGE("Event subscriber unregistration failed with error %d", err);
}


mdv_errno mdv_ebus_publish(mdv_ebus *ebus,
                           mdv_event *event,
                           int flags)
{
    if (event->type >= ebus->events_count)
    {
        MDV_LOGE("Invalid event type");
        return MDV_FAILED;
    }

    if(flags & MDV_EVT_SYNC)
    {
        return mdv_ebus_event_process(ebus, event);
    }
    else if(flags & MDV_EVT_UNIQUE)
    {
        uint_fast32_t expected = 0;
        if (!atomic_compare_exchange_strong(ebus->events_gen + event->type, &expected, 1))
            return MDV_EEXIST;
    }
    else
    {
        atomic_fetch_add_explicit(ebus->events_gen + event->type, 1, memory_order_acquire);
    }

    // MDV_EVT_DEFAULT or MDV_EVT_UNIQUE

    size_t idx = atomic_load_explicit(&ebus->idx, memory_order_relaxed);

    while (!atomic_compare_exchange_weak(&ebus->idx,
                                         &idx,
                                         (idx + 1) % ebus->size));

    idx = (idx + 1) % ebus->size;

    event->vptr->retain(event);

    for(size_t i = 0; i < MDV_EBUS_QUEUE_PUSH_ATTEMPTS; ++i)
    {
        if (mdv_queuefd_push(ebus->queues[idx], event))
        {
            return MDV_OK;
        }
        mdv_thread_yield();
    }

    event->vptr->release(event);

    return MDV_FAILED;
}
