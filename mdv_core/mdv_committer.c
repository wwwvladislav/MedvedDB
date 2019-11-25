#include "mdv_committer.h"
#include "event/mdv_evt_types.h"
#include "event/mdv_evt_topology.h"
#include "event/mdv_evt_trlog.h"
#include <stdatomic.h>
#include <mdv_alloc.h>
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <mdv_eventfd.h>
#include <mdv_uuid.h>
#include <mdv_threads.h>
#include <mdv_safeptr.h>


/// Data committer
struct mdv_committer
{
    atomic_uint     rc;             ///< References counter
    mdv_ebus       *ebus;           ///< Event bus
    mdv_jobber     *jobber;         ///< Jobs scheduler
    mdv_descriptor  start;          ///< Signal for starting
    mdv_thread      thread;         ///< Committer thread
    atomic_bool     active;         ///< Status
    atomic_size_t   active_jobs;    ///< Active jobs counter
    mdv_safeptr    *topology;       ///< Current network topology
};


typedef struct mdv_committer_context
{
    mdv_committer  *committer;      ///< Data committer
    mdv_uuid        storage;        ///< Data storage UUID for commit
} mdv_committer_context;


typedef mdv_job(mdv_committer_context)     mdv_committer_job;


static bool mdv_committer_is_active(mdv_committer *committer)
{
    return atomic_load(&committer->active);
}


static void mdv_committer_fn(mdv_job_base *job)
{
    mdv_committer_context *ctx      = (mdv_committer_context *)job->data;
    mdv_committer         *committer = ctx->committer;

    if (!mdv_committer_is_active(committer))
        return;

    mdv_evt_trlog_apply *apply = mdv_evt_trlog_apply_create(&ctx->storage);

    if (apply)
    {
        if (mdv_ebus_publish(committer->ebus, &apply->base, MDV_EVT_SYNC) != MDV_OK)
            MDV_LOGE("Transaction log was not applied");
        mdv_evt_trlog_apply_release(apply);
    }
}


static void mdv_committer_finalize(mdv_job_base *job)
{
    mdv_committer_context *ctx = (mdv_committer_context *)job->data;
    mdv_committer         *committer = ctx->committer;
    mdv_committer_release(committer);
    atomic_fetch_sub_explicit(&committer->active_jobs, 1, memory_order_relaxed);
    mdv_free(job, "committer_job");
}


static void mdv_committer_job_emit(mdv_committer *committer, mdv_uuid const *storage)
{
    mdv_committer_job *job = mdv_alloc(sizeof(mdv_committer_job), "committer_job");

    if (!job)
    {
        MDV_LOGE("No memory for data committer job");
        return;
    }

    job->fn             = mdv_committer_fn;
    job->finalize       = mdv_committer_finalize;
    job->data.committer = mdv_committer_retain(committer);
    job->data.storage   = *storage;

    mdv_errno err = mdv_jobber_push(committer->jobber, (mdv_job_base*)job);

    if (err != MDV_OK)
    {
        MDV_LOGE("Data committer job failed");
        mdv_committer_release(committer);
        mdv_free(job, "committer_job");
    }
    else
    {
        atomic_fetch_add_explicit(&committer->active_jobs, 1, memory_order_relaxed);
    }
}


// Main data commit thread generates asynchronous jobs for each peer.
static void mdv_committer_main(mdv_committer *committer)
{
    mdv_topology *topology = mdv_safeptr_get(committer->topology);

    if (topology)
    {
        mdv_vector *nodes = mdv_topology_nodes(topology);

        if (nodes)
        {
            mdv_vector_foreach(nodes, mdv_toponode, node)
            {
                mdv_committer_job_emit(committer, &node->uuid);
            }

            mdv_vector_release(nodes);
        }

        mdv_topology_release(topology);
    }
}


static void * mdv_committer_thread(void *arg)
{
    mdv_committer *committer = arg;

    atomic_store(&committer->active, true);

    mdv_descriptor epfd = mdv_epoll_create();

    if (epfd == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("epoll creation failed");
        return 0;
    }

    mdv_epoll_event const event =
    {
        .events = MDV_EPOLLIN | MDV_EPOLLERR,
        .data = 0
    };

    if (mdv_epoll_add(epfd, committer->start, event) != MDV_OK)
    {
        MDV_LOGE("epoll event registration failed");
        mdv_epoll_close(epfd);
        return 0;
    }

    while(mdv_committer_is_active(committer))
    {
        mdv_epoll_event events[1];
        uint32_t size = sizeof events / sizeof *events;

        mdv_epoll_wait(epfd, events, &size, -1);

        if (!mdv_committer_is_active(committer))
            break;

        if (size)
        {
            if (events[0].events & MDV_EPOLLIN)
            {
                uint64_t signal;
                size_t len = sizeof(signal);

                while(mdv_read(committer->start, &signal, &len) == MDV_EAGAIN);

                mdv_committer_main(committer);
            }
        }
    }

    mdv_epoll_close(epfd);

    return 0;
}


static mdv_errno mdv_committer_evt_topology(void *arg, mdv_event *event)
{
    mdv_committer *committer = arg;
    mdv_evt_topology *topo = (mdv_evt_topology *)event;
    return mdv_safeptr_set(committer->topology, topo->topology);
}


static mdv_errno mdv_committer_evt_trlog_changed(void *arg, mdv_event *event)
{
    mdv_committer *committer = arg;
    mdv_evt_trlog_changed *evt = (mdv_evt_trlog_changed *)event;
    (void)evt;
    mdv_committer_start(committer);
    return MDV_OK;
}


static const mdv_event_handler_type mdv_committer_handlers[] =
{
    { MDV_EVT_TOPOLOGY,         mdv_committer_evt_topology },
    { MDV_EVT_TRLOG_CHANGED,    mdv_committer_evt_trlog_changed },
};


mdv_committer * mdv_committer_create(mdv_ebus *ebus, mdv_jobber_config const *jconfig, mdv_topology *topology)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(6);

    mdv_committer *committer = mdv_alloc(sizeof(mdv_committer), "committer");

    if (!committer)
    {
        MDV_LOGE("No memory for new committer");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, committer, "committer");

    atomic_init(&committer->rc, 1);
    atomic_init(&committer->active_jobs, 0);
    atomic_init(&committer->active, false);

    committer->ebus = mdv_ebus_retain(ebus);

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, committer->ebus);

    committer->topology = mdv_safeptr_create(topology,
                                        (mdv_safeptr_retain_fn)mdv_topology_retain,
                                        (mdv_safeptr_release_fn)mdv_topology_release);

    if (!committer->topology)
    {
        MDV_LOGE("Safe pointer creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_safeptr_free, committer->topology);

    committer->jobber = mdv_jobber_create(jconfig);

    if (!committer->jobber)
    {
        MDV_LOGE("Jobs scheduler creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_jobber_release, committer->jobber);

    committer->start = mdv_eventfd(false);

    if (committer->start == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("Eventfd creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_eventfd_close, committer->start);

    mdv_thread_attrs const attrs =
    {
        .stack_size = MDV_THREAD_STACK_SIZE
    };

    mdv_errno err = mdv_thread_create(&committer->thread, &attrs, mdv_committer_thread, committer);

    if (err != MDV_OK)
    {
        MDV_LOGE("Thread creation failed with error %d", err);
        mdv_rollback(rollbacker);
        return 0;
    }

    while(!mdv_committer_is_active(committer))
        mdv_sleep(100);

    mdv_rollbacker_push(rollbacker, mdv_committer_stop, committer);

    if (mdv_ebus_subscribe_all(committer->ebus,
                               committer,
                               mdv_committer_handlers,
                               sizeof mdv_committer_handlers / sizeof *mdv_committer_handlers) != MDV_OK)
    {
        MDV_LOGE("Ebus subscription failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_free(rollbacker);

    return committer;
}


void mdv_committer_start(mdv_committer *committer)
{
    uint64_t const signal = 1;
    mdv_write_all(committer->start, &signal, sizeof signal);
}


void mdv_committer_stop(mdv_committer *committer)
{
    if (mdv_committer_is_active(committer))
    {
        atomic_store(&committer->active, false);

        uint64_t const signal = 1;
        mdv_write_all(committer->start, &signal, sizeof signal);

        mdv_thread_join(committer->thread);

        while(atomic_load_explicit(&committer->active_jobs, memory_order_relaxed) > 0)
            mdv_sleep(100);
    }
}


static void mdv_committer_free(mdv_committer *committer)
{
    mdv_committer_stop(committer);

    mdv_ebus_unsubscribe_all(committer->ebus,
                             committer,
                             mdv_committer_handlers,
                             sizeof mdv_committer_handlers / sizeof *mdv_committer_handlers);

    mdv_jobber_release(committer->jobber);
    mdv_eventfd_close(committer->start);
    mdv_ebus_release(committer->ebus);
    mdv_safeptr_free(committer->topology);
    memset(committer, 0, sizeof(*committer));
    mdv_free(committer, "committer");
}


mdv_committer * mdv_committer_retain(mdv_committer *committer)
{
    atomic_fetch_add_explicit(&committer->rc, 1, memory_order_acquire);
    return committer;
}


uint32_t mdv_committer_release(mdv_committer *committer)
{
    if (!committer)
        return 0;

    uint32_t rc = atomic_fetch_sub_explicit(&committer->rc, 1, memory_order_release) - 1;

    if (!rc)
        mdv_committer_free(committer);

    return rc;
}
