#include "mdv_buffer_manager.h"
#include <mdv_log.h>
#include <mdv_file.h>
#include <mdv_hashmap.h>
#include <mdv_lrucache.h>
#include <mdv_limits.h>
#include <mdv_assert.h>
#include <mdv_alloc.h>
#include <mdv_mutex.h>
#include <mdv_vector.h>
#include <mdv_filesystem.h>
#include <mdv_rollbacker.h>
#include <stdatomic.h>


/// Buffers storage
typedef struct mdv_buffers_storage
{
    atomic_uint_fast32_t rc;                    ///< References counter
    mdv_descriptor       fd;                    ///< File associated with buffer
} mdv_buffers_storage;


/// Buffer metai nformation
typedef struct mdv_buffer_metainf
{
    uint16_t pinned:1;                          ///< buffer cannot at the moment be written back to disk safely (in use)
    uint16_t dirty:1;                           ///< buffer has been changed
    uint16_t compressed:1;                      ///< buffer compressed
    uint16_t compressed_size;                   ///< compressed buffer size (must be less or equal MDV_BUFFER_SIZE)
    uint32_t size;                              ///< buffer size
    uint32_t reserved[2];                       ///< reserved
} mdv_buffer_metainf;


/// Buffer runtime information
typedef struct mdv_buffer_runtime
{
    mdv_buffer_id        id;                    ///< Buffer identifier
    mdv_buffers_storage *storage;               ///< Buffer storage
} mdv_buffer_runtime;


/// Buffer
struct mdv_buffer
{
    mdv_buffer_runtime runtime;                 ///< Buffer runtime information
    struct
    {
        mdv_buffer_metainf header;              ///< Buffer metainformation
        uint8_t            ptr[1];              ///< Data pointer
    } data;                                     ///< Data
};


mdv_static_assert(sizeof(mdv_buffer_metainf) == 16u);
mdv_static_assert(offsetof(mdv_buffer, data.ptr)
                    == offsetof(mdv_buffer, data.header)
                        + sizeof(mdv_buffer_metainf));


struct mdv_buffer_manager
{
    atomic_uint_fast32_t rc;                    ///< References counter
    mdv_hashmap         *storages;              ///< Active storages
    mdv_mutex            storages_mutex;        ///< Mutex for storages guard
    mdv_lrucache        *buffers;               ///< Memory mapped buffers
    mdv_mutex            buffers_mutex;         ///< Mutex for buffers guard
    mdv_descriptor       free_buffers_fd;       ///< File where free buffers identifiers is stored
    mdv_vector          *free_buffers;          ///< Free buffers identifiers
    mdv_mutex            free_buffers_mutex;    ///< Mutex for free buffers identifiers guard
};


mdv_buffer_manager * mdv_buffer_manager_open(size_t capacity, char const *dir)
{
    // Create buffers persist directory
    if (!mdv_mkdir(dir))
    {
        MDV_LOGE("Buffers persist directory creation failed: '%s'", dir);
        return 0;
    }

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(5);

    mdv_buffer_manager *buffer_manager = mdv_alloc(sizeof(mdv_buffer_manager), "buffer_manager");

    if(!buffer_manager)
    {
        MDV_LOGE("No memory for new buffers manager");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, buffer_manager, "buffer_manager");

    buffer_manager->storages = mdv_hashmap_create(mdv_buffers_storage,
                                                  fd,
                                                  4,
                                                  mdv_descriptor_hash,
                                                  mdv_descriptor_cmp);

    if (!buffer_manager->storages)
    {
        MDV_LOGE("No memory for new buffers storages map");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, buffer_manager->storages);

    if (mdv_mutex_create(&buffer_manager->storages_mutex))
    {
        MDV_LOGE("Mutex creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &buffer_manager->storages_mutex);

    buffer_manager->buffers = mdv_lrucache_create(capacity);

    if(!buffer_manager->buffers)
    {
        MDV_LOGE("No memory for shared buffers cache");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_lrucache_release, buffer_manager->buffers);

    if (mdv_mutex_create(&buffer_manager->buffers_mutex))
    {
        MDV_LOGE("Mutex creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &buffer_manager->buffers_mutex);

    // TODO

    mdv_rollbacker_free(rollbacker);

    return buffer_manager;
}
