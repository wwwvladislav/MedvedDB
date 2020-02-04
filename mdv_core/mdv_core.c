#include "mdv_core.h"
#include "mdv_user.h"
#include "mdv_peer.h"
#include "mdv_config.h"
#include "mdv_p2pmsg.h"
#include "mdv_syncer.h"
#include "mdv_committer.h"
#include "mdv_fetcher.h"
#include "mdv_conman.h"
#include "mdv_tracker.h"
#include "storage/mdv_metainf.h"
#include "storage/mdv_tablespace.h"
#include "event/mdv_evt_types.h"
#include <mdv_log.h>
#include <mdv_messages.h>
#include <mdv_proto.h>
#include <mdv_rollbacker.h>
#include <mdv_jobber.h>
#include <mdv_ebus.h>


struct mdv_core
{
    mdv_ebus       *ebus;               ///< Events bus
    mdv_tracker    *tracker;            ///< Network topology tracker
    mdv_conman     *conman;             ///< Connections manager
    mdv_metainf     metainf;            ///< Metainformation (DB version, node UUID etc.)
    mdv_syncer     *syncer;             ///< Data synchronizer
    mdv_committer  *committer;          ///< Data committer
    mdv_fetcher    *fetcher;            ///< Data fetcher

    struct
    {
        mdv_storage    *metainf;        ///< Metainformation storage
        mdv_tablespace *tablespace;     ///< Tables storage
    } storage;                          ///< Storages
};


mdv_core * mdv_core_create()
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(10);

    mdv_core *core = mdv_alloc(sizeof(mdv_core), "core");

    if (!core)
    {
        MDV_LOGE("New memory for core");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, core, "core");


    // DB meta information storage
    core->storage.metainf = mdv_metainf_storage_open(MDV_CONFIG.storage.path.ptr);

    if (!core->storage.metainf)
    {
        MDV_LOGE("Service initialization failed. Can't create metainf storage '%s'", MDV_CONFIG.storage.path.ptr);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_storage_release, core->storage.metainf);


    if (!mdv_metainf_load(&core->metainf, core->storage.metainf))
    {
        MDV_LOGE("DB meta information loading failed. Path: '%s'", MDV_CONFIG.storage.path.ptr);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_metainf_validate(&core->metainf);

    mdv_metainf_flush(&core->metainf, core->storage.metainf);

    // Events bus
    mdv_ebus_config const ebus_config =
    {
        .threadpool =
        {
            .size = MDV_CONFIG.ebus.workers,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
        .event =
        {
            .queues_count = MDV_CONFIG.ebus.queues,
            .max_id = MDV_EVT_COUNT
        }
    };

    core->ebus = mdv_ebus_create(&ebus_config);

    if (!core->ebus)
    {
        MDV_LOGE("Events bus creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_ebus_release, core->ebus);


    // Topology tracker
    core->tracker = mdv_tracker_create(&core->metainf.uuid.value,
                                       core->storage.metainf,
                                       core->ebus);

    if (!core->tracker)
    {
        MDV_LOGE("Topology tracker creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_tracker_release, core->tracker);

    mdv_topology *topology = mdv_tracker_topology(core->tracker);

    mdv_rollbacker_push(rollbacker, mdv_topology_release, topology);


    // Tablespace
    core->storage.tablespace = mdv_tablespace_open(&core->metainf.uuid.value, core->ebus, topology);

    if (!core->storage.tablespace)
    {
        MDV_LOGE("DB tables space creation failed. Path: '%s'", MDV_CONFIG.storage.path.ptr);
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_tablespace_close, core->storage.tablespace);


    // Data synchronizer
    {
        mdv_jobber_config const jconfig =
        {
            .threadpool =
            {
                .size = MDV_CONFIG.datasync.workers,
                .thread_attrs =
                {
                    .stack_size = MDV_THREAD_STACK_SIZE
                }
            },
            .queue =
            {
                .count = MDV_CONFIG.datasync.queues
            }
        };

        core->syncer = mdv_syncer_create(&core->metainf.uuid.value, core->ebus, &jconfig, topology);
    }

    if (!core->syncer)
    {
        MDV_LOGE("Data synchronizer creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_syncer_release, core->syncer);


    // Data committer
    {
        mdv_jobber_config const jconfig =
        {
            .threadpool =
            {
                .size = MDV_CONFIG.committer.workers,
                .thread_attrs =
                {
                    .stack_size = MDV_THREAD_STACK_SIZE
                }
            },
            .queue =
            {
                .count = MDV_CONFIG.committer.queues
            }
        };

        core->committer = mdv_committer_create(core->ebus, &jconfig, topology);
    }

    if (!core->committer)
    {
        MDV_LOGE("Data committer creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_committer_release, core->committer);


    // Data fetcher
    {
        mdv_jobber_config const jconfig =
        {
            .threadpool =
            {
                .size = MDV_CONFIG.fetcher.workers,
                .thread_attrs =
                {
                    .stack_size = MDV_THREAD_STACK_SIZE
                }
            },
            .queue =
            {
                .count = MDV_CONFIG.fetcher.queues
            }
        };

        core->fetcher = mdv_fetcher_create(core->ebus, &jconfig);
    }

    if (!core->fetcher)
    {
        MDV_LOGE("Data fetcher creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_fetcher_release, core->fetcher);


    // Connections manager
    mdv_conman_config const conman_config =
    {
        .uuid = core->metainf.uuid.value,
        .channel =
        {
            .retry_interval = MDV_CONFIG.connection.retry_interval,
            .keepidle       = MDV_CONFIG.connection.keep_idle,
            .keepcnt        = MDV_CONFIG.connection.keep_count,
            .keepintvl      = MDV_CONFIG.connection.keep_interval
        },
        .threadpool =
        {
            .size = MDV_CONFIG.server.workers,
            .thread_attrs =
            {
                .stack_size = MDV_THREAD_STACK_SIZE
            }
        },
    };

    core->conman = mdv_conman_create(&conman_config, core->ebus);

    if (!core->conman)
    {
        MDV_LOGE("Cluster manager creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_conman_free, core->conman);


    MDV_LOGI("Storage version: %u", core->metainf.version.value);

    MDV_LOGI("Node UUID: %s", mdv_uuid_to_str(&core->metainf.uuid.value).ptr);

    mdv_rollbacker_free(rollbacker);

    mdv_topology_release(topology);

    return core;
}


void mdv_core_free(mdv_core *core)
{
    if (core)
    {
        mdv_tracker_release(core->tracker);
        mdv_syncer_cancel(core->syncer);
        mdv_committer_stop(core->committer);
        mdv_syncer_release(core->syncer);
        mdv_conman_free(core->conman);
        mdv_committer_release(core->committer);
        mdv_fetcher_release(core->fetcher);
        mdv_storage_release(core->storage.metainf);
        mdv_tablespace_close(core->storage.tablespace);
        mdv_ebus_release(core->ebus);
        mdv_free(core, "core");
    }
}


bool mdv_core_listen(mdv_core *core)
{
    return mdv_conman_bind(core->conman, MDV_CONFIG.server.listen) == MDV_OK;
}


void mdv_core_connect(mdv_core *core)
{
    for(uint32_t i = 0; i < MDV_CONFIG.cluster.size; ++i)
    {
        mdv_conman_connect(core->conman,
                           mdv_str((char*)MDV_CONFIG.cluster.nodes[i]),
                           MDV_PEER_CHANNEL);
    }
}

