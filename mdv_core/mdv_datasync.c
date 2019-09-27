#include "mdv_datasync.h"
#include <mdv_alloc.h>
#include <mdv_errno.h>
#include <mdv_log.h>
#include <mdv_rollbacker.h>
#include <mdv_epoll.h>
#include <mdv_assert.h>
#include "mdv_p2pmsg.h"
#include "mdv_peer.h"


typedef struct mdv_datasync_context
{
    uint32_t        peer_src;       ///< Source peer identifier
    mdv_vector     *routes;         ///< Destination peers
    mdv_datasync   *datasync;       ///< Data synchronizer
    mdv_uuid        storage;        ///< Data storage UUID for synchronization
} mdv_datasync_context;


typedef mdv_job(mdv_datasync_context)     mdv_datasync_job;


static bool mdv_datasync_is_active(mdv_datasync *datasync)
{
    return atomic_load(&datasync->active);
}


static mdv_errno mdv_datasync_cfslog_sync(mdv_datasync   *datasync,
                                          mdv_uuid const *storage_uuid,
                                          uint32_t        peer_src,
                                          mdv_vector     *routes)
{
    mdv_tracker *tracker = datasync->tracker;

    mdv_node const *node_src = mdv_tracker_node_by_id(tracker, peer_src);

    if (!node_src)
        return MDV_FAILED;

    mdv_msg_p2p_cfslog_sync const sync =
    {
        .uuid = *storage_uuid,
        .peer = node_src->uuid
    };

    binn obj;

    if (mdv_binn_p2p_cfslog_sync(&sync, &obj))
    {
        mdv_msg message =
        {
            .hdr =
            {
                .id = mdv_message_id(p2p_cfslog_sync),
                .size = binn_size(&obj)
            },
            .payload = binn_ptr(&obj)
        };

        mdv_vector_foreach(routes, uint32_t, dst)
        {
            if (peer_src == *dst)
                continue;
            mdv_tracker_peers_call(tracker, *dst, &message, &mdv_peer_node_post);
        }

        binn_free(&obj);
    }
    else
        return MDV_FAILED;

    return MDV_OK;
}



static mdv_errno mdv_datasync_cfslog_state(mdv_datasync                   *datasync,
                                           uint32_t                        peer_id,
                                           mdv_msg_p2p_cfslog_state const *cfslog_state)
{
    mdv_tracker *tracker = datasync->tracker;

    binn obj;

    if (mdv_binn_p2p_cfslog_state(cfslog_state, &obj))
    {
        mdv_msg message =
        {
            .hdr =
            {
                .id = mdv_message_id(p2p_cfslog_state),
                .size = binn_size(&obj)
            },
            .payload = binn_ptr(&obj)
        };

        mdv_tracker_peers_call(tracker, peer_id, &message, &mdv_peer_node_post);

        binn_free(&obj);
    }
    else
        return MDV_FAILED;

    return MDV_OK;
}


mdv_errno mdv_datasync_cfslog_sync_handler(mdv_datasync *datasync, uint32_t peer_id, mdv_msg const *msg)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_p2p_msg_name(msg->hdr.id));
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &binn_msg);

    mdv_msg_p2p_cfslog_sync cfslog_sync;

    if (!mdv_unbinn_p2p_cfslog_sync(&binn_msg, &cfslog_sync))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_p2p_msg_name(msg->hdr.id));
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_cfstorage *storage = mdv_tablespace_cfstorage(datasync->tablespace, &cfslog_sync.uuid);

    mdv_node *node = mdv_tracker_node_by_uuid(datasync->tracker, &cfslog_sync.peer);

    if (storage && node)
    {
        mdv_msg_p2p_cfslog_state const cfslog_state =
        {
            .uuid = cfslog_sync.uuid,
            .peer = cfslog_sync.peer,
            .trlog_top = mdv_cfstorage_log_last_id(storage, node->id)
        };

        if (mdv_datasync_cfslog_state(datasync, peer_id, &cfslog_state) != MDV_OK)
            MDV_LOGE("Cflog state notification failed");
    }

    mdv_rollback(rollbacker);

    return MDV_OK;
}


typedef struct
{
    mdv_datasync   *datasync;       ///< Data synchronizer
    mdv_uuid        uuid;           ///< CF Storage unique identifier
    mdv_uuid        peer;           ///< Peer unique identifier
} mdv_datasync_cfs_context;


static bool mdv_datasync_cfs(void           *arg,
                             uint32_t        peer_src,
                             uint32_t        peer_dst,
                             size_t          count,
                             mdv_list const *ops)       // mdv_cfstorage_op ops[]
{
    mdv_datasync_cfs_context *ctx = arg;

    mdv_static_assert(sizeof(mdv_cfslog_data) == sizeof(mdv_cfstorage_op));

    mdv_msg_p2p_cfslog_data cfslog_data =
    {
        .uuid = ctx->uuid,
        .peer = ctx->peer,
        .count = count,
        .rows = (mdv_list*)ops
    };

    binn obj;

    if (mdv_binn_p2p_cfslog_data(&cfslog_data, &obj))
    {
        mdv_msg message =
        {
            .hdr =
            {
                .id = mdv_message_id(p2p_cfslog_data),
                .size = binn_size(&obj)
            },
            .payload = binn_ptr(&obj)
        };

        mdv_tracker_peers_call(ctx->datasync->tracker, peer_dst, &message, &mdv_peer_node_post);

        binn_free(&obj);
    }
    else
        return false;

    return true;
}


mdv_errno mdv_datasync_cfslog_state_handler(mdv_datasync *datasync,
                                            uint32_t peer_id,
                                            mdv_msg const *msg)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(1);

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_p2p_msg_name(msg->hdr.id));
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &binn_msg);

    mdv_msg_p2p_cfslog_state cfslog_state;

    if (!mdv_unbinn_p2p_cfslog_state(&binn_msg, &cfslog_state))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_p2p_msg_name(msg->hdr.id));
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_cfstorage *storage = mdv_tablespace_cfstorage(datasync->tablespace, &cfslog_state.uuid);

    mdv_node *node = mdv_tracker_node_by_uuid(datasync->tracker, &cfslog_state.peer);

    if (storage && node)
    {
        mdv_datasync_cfs_context ctx =
        {
            .datasync = datasync,
            .uuid = cfslog_state.uuid,
            .peer = cfslog_state.peer
        };

        mdv_cfstorage_sync(storage,
                           cfslog_state.trlog_top,
                           node->id,           // peer_src
                           peer_id,        // peer_dst
                           &ctx,
                           mdv_datasync_cfs);
    }

    mdv_rollback(rollbacker);

    return MDV_OK;
}


mdv_errno mdv_datasync_cfslog_data_handler(mdv_datasync  *datasync,
                                           uint32_t       peer_id,
                                           mdv_msg const *msg)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    binn binn_msg;

    if(!binn_load(msg->payload, &binn_msg))
    {
        MDV_LOGW("Message '%s' reading failed", mdv_p2p_msg_name(msg->hdr.id));
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, binn_free, &binn_msg);

    mdv_list rows = {};

    mdv_uuid const *storage_uuid = mdv_unbinn_p2p_cfslog_data_uuid(&binn_msg);
    mdv_uuid const *peer_uuid = mdv_unbinn_p2p_cfslog_data_peer(&binn_msg);
    uint32_t const *rows_count = mdv_unbinn_p2p_cfslog_data_count(&binn_msg);

    if (mdv_unbinn_p2p_cfslog_data_rows(&binn_msg, &rows))
    {
        mdv_rollbacker_push(rollbacker, mdv_list_clear, &rows);

        if (storage_uuid
            && peer_uuid
            && rows_count)
        {
            mdv_node const *node = mdv_tracker_node_by_uuid(datasync->tracker, peer_uuid);

            if (node)
            {
                mdv_cfstorage *storage = mdv_tablespace_cfstorage(datasync->tablespace, storage_uuid);

                if (storage)
                {
                    // TODO:
                }
                else
                    MDV_LOGE("Unknown storage: %s", mdv_uuid_to_str(storage_uuid).ptr);
            }
            else
                MDV_LOGE("Unknown node: %s", mdv_uuid_to_str(peer_uuid).ptr);
        }
        else
            MDV_LOGE("Missing data in sync message");
    }

    mdv_rollback(rollbacker);

    return MDV_OK;
}


static void mdv_datasync_fn(mdv_job_base *job)
{
    mdv_datasync_context *ctx      = (mdv_datasync_context *)job->data;
    mdv_datasync         *datasync = ctx->datasync;

    if (!mdv_datasync_is_active(datasync))
        return;

    mdv_datasync_cfslog_sync(datasync, &ctx->storage, ctx->peer_src, ctx->routes);
}


static void mdv_datasync_finalize(mdv_job_base *job)
{
    mdv_datasync_context *ctx = (mdv_datasync_context *)job->data;
    mdv_datasync         *datasync = ctx->datasync;
    mdv_vector_release(ctx->routes);
    atomic_fetch_sub_explicit(&datasync->active_jobs, 1, memory_order_relaxed);
    mdv_free(job, "datasync_job");
}


static mdv_vector * mdv_datasync_routes(mdv_datasync *datasync)
{
    mdv_vector *routes = 0;

    if (mdv_mutex_lock(&datasync->mutex) == MDV_OK)
    {
        routes = mdv_vector_retain(datasync->routes);
        mdv_mutex_unlock(&datasync->mutex);
    }

    return routes;
}


static void mdv_datasync_job_emit(mdv_datasync *datasync, uint32_t peer_src, mdv_vector *routes, mdv_uuid const *storage)
{
    mdv_datasync_job *job = mdv_alloc(sizeof(mdv_datasync_job), "datasync_job");

    if (!job)
    {
        MDV_LOGE("No memory for data synchronization job");
        return;
    }

    job->fn             = mdv_datasync_fn;
    job->finalize       = mdv_datasync_finalize;
    job->data.peer_src  = peer_src;
    job->data.routes    = mdv_vector_retain(routes);
    job->data.datasync  = datasync;
    job->data.storage   = *storage;

    mdv_errno err = mdv_jobber_push(datasync->jobber, (mdv_job_base*)job);

    if (err != MDV_OK)
    {
        MDV_LOGE("Data synchronization job failed");
        mdv_free(job, "datasync_job");
    }
    else
    {
        atomic_fetch_add_explicit(&datasync->active_jobs, 1, memory_order_relaxed);
    }
}


// Main data synchronization thread generates asynchronous jobs for each peer.
static void mdv_datasync_main(mdv_datasync *datasync)
{
    mdv_vector *src_peers = mdv_tracker_nodes(datasync->tracker);

    if (!src_peers
        || mdv_vector_empty(src_peers))
        return;

    mdv_vector *routes = mdv_datasync_routes(datasync);

    if (!routes
        || mdv_vector_empty(routes))
    {
        mdv_vector_release(src_peers);
        return;
    }

    mdv_vector_foreach(src_peers, uint32_t, src)
    {
        mdv_hashmap_foreach(datasync->tablespace->storages, mdv_cfstorage_ref, ref)
        {
            mdv_datasync_job_emit(datasync, *src, routes, &ref->uuid);
        }
    }

    mdv_vector_release(routes);
    mdv_vector_release(src_peers);
}


static void * mdv_datasync_thread(void *arg)
{
    mdv_datasync *datasync = arg;

    atomic_store(&datasync->active, true);

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

    if (mdv_epoll_add(epfd, datasync->start, event) != MDV_OK)
    {
        MDV_LOGE("epoll event registration failed");
        mdv_epoll_close(epfd);
        return 0;
    }

    while(mdv_datasync_is_active(datasync))
    {
        mdv_epoll_event events[1];
        uint32_t size = sizeof events / sizeof *events;

        mdv_epoll_wait(epfd, events, &size, -1);

        if (!mdv_datasync_is_active(datasync))
            break;

        if (size)
        {
            if (events[0].events & MDV_EPOLLIN)
            {
                uint64_t signal;
                size_t len = sizeof(signal);

                while(mdv_read(datasync->start, &signal, &len) == MDV_EAGAIN);

                // Synchronization start
                mdv_datasync_main(datasync);
            }
        }
    }

    mdv_epoll_close(epfd);

    return 0;
}


void mdv_datasync_stop(mdv_datasync *datasync)
{
    if (mdv_datasync_is_active(datasync))
    {
        atomic_store(&datasync->active, false);

        uint64_t const signal = 1;
        mdv_write_all(datasync->start, &signal, sizeof signal);

        mdv_thread_join(datasync->thread);

        while(atomic_load_explicit(&datasync->active_jobs, memory_order_relaxed) > 0)
            mdv_sleep(100);
    }
}


mdv_errno mdv_datasync_create(mdv_datasync *datasync,
                              mdv_tablespace *tablespace,
                              mdv_tracker *tracker,
                              mdv_jobber *jobber)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(4);

    datasync->tablespace = tablespace;
    datasync->tracker = tracker;
    datasync->jobber = jobber;
    datasync->routes = 0;

    atomic_init(&datasync->active_jobs, 0);

    mdv_errno err = mdv_mutex_create(&datasync->mutex);

    if (err != MDV_OK)
    {
        MDV_LOGE("Mutex creation failed with error %d", err);
        mdv_rollback(rollbacker);
        return err;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &datasync->mutex);

    datasync->start = mdv_eventfd(false);

    if (datasync->start == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("Eventfd creation failed");
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_eventfd_close, datasync->start);

    atomic_init(&datasync->active, false);

    mdv_thread_attrs const attrs =
    {
        .stack_size = MDV_THREAD_STACK_SIZE
    };

    err = mdv_thread_create(&datasync->thread, &attrs, mdv_datasync_thread, datasync);

    if (err != MDV_OK)
    {
        MDV_LOGE("Thread creation failed with error %d", err);
        mdv_rollback(rollbacker);
        return err;
    }

    while(!mdv_datasync_is_active(datasync))
        mdv_sleep(100);

    mdv_rollbacker_push(rollbacker, mdv_datasync_stop, datasync);

    mdv_rollbacker_free(rollbacker);

    return MDV_OK;
}


void mdv_datasync_free(mdv_datasync *datasync)
{
    mdv_datasync_stop(datasync);
    mdv_vector_release(datasync->routes);
    mdv_mutex_free(&datasync->mutex);
    mdv_eventfd_close(datasync->start);
    memset(datasync, 0, sizeof(*datasync));
}


mdv_errno mdv_datasync_update_routes(mdv_datasync *datasync)
{
    if (!mdv_datasync_is_active(datasync))
        return MDV_FAILED;

    mdv_vector *routes = mdv_routes_find(datasync->tracker);

    if (!routes)
    {
        MDV_LOGE("No memorty for routes");
        return MDV_NO_MEM;
    }

    if (mdv_mutex_lock(&datasync->mutex) == MDV_OK)
    {
        mdv_vector_release(datasync->routes);

        datasync->routes = routes;

        MDV_LOGD("New routes count=%zu", mdv_vector_size(routes));

        for(size_t i = 0; i < mdv_vector_size(routes); ++i)
        {
            MDV_LOGD("Route[%zu]: %u", i, *(uint32_t*)mdv_vector_at(routes, i));
        }

        mdv_mutex_unlock(&datasync->mutex);

        return MDV_OK;
    }

    return MDV_FAILED;
}


void mdv_datasync_start(mdv_datasync *datasync)
{
    uint64_t const signal = 1;
    mdv_write_all(datasync->start, &signal, sizeof signal);
}
