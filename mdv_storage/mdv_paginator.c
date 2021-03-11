#include "mdv_paginator.h"
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


/* Storage format
  ______________________________________
 |                                      |
 | 8KB area where page                  |
 | numers stored which                  |
 | contain free page identifiers        |
  --------------------------------------
 |                                      |
 | 8KB data or page mentioned           |
 | in header                            |
  --------------------------------------
 |                                      |
 | 8KB data or page mentioned           |
 | in header                            |
  --------------------------------------
  ...
*/


/// Pages storage
typedef struct mdv_pages_storage
{
    atomic_uint_fast32_t rc;                        ///< References counter
    mdv_descriptor       fd;                        ///< File associated with page
    mdv_vector          *free_pages;                ///< Free page identifiers
} mdv_pages_storage;


/// Buffer
struct mdv_buffer
{
    mdv_buffer_id_t      id;                        ///< Buffer identifier
    mdv_pages_storage   *storage;                   ///< Buffer storage
    uint16_t             pinned:1;                  ///< buffer cannot at the moment be written back to disk safely (in use)
    uint16_t             dirty:1;                   ///< buffer has been changed
    uint32_t             size;                      ///< Data size
    uint8_t              data[1];                   ///< Data
};


struct mdv_paginator
{
    atomic_uint_fast32_t rc;                        ///< References counter
    mdv_hashmap         *storages;                  ///< Active storages
    mdv_mutex            storages_mutex;            ///< Mutex for storages guard
    mdv_lrucache        *buffers;                   ///< Memory mapped buffers
    mdv_mutex            buffers_mutex;             ///< Mutex for buffers guard
    size_t               page_size;                 ///< Page size
};


mdv_paginator * mdv_paginator_open(size_t capacity, size_t page_size, char const *dir)
{
    // Create buffers persist directory
    if (!mdv_mkdir(dir))
    {
        MDV_LOGE("Buffers persist directory creation failed: '%s'", dir);
        return 0;
    }

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(5);

    mdv_paginator *paginator = mdv_alloc(sizeof(mdv_paginator), "paginator");

    if (!paginator)
    {
        MDV_LOGE("No memory for new buffers manager");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, paginator, "paginator");

    paginator->storages = mdv_hashmap_create(mdv_pages_storage,
                                             fd,
                                             4,
                                             mdv_descriptor_hash,
                                             mdv_descriptor_cmp);

    if (!paginator->storages)
    {
        MDV_LOGE("No memory for new buffers storages map");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_hashmap_release, paginator->storages);

    if (mdv_mutex_create(&paginator->storages_mutex) != MDV_OK)
    {
        MDV_LOGE("Mutex creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &paginator->storages_mutex);

    paginator->buffers = mdv_lrucache_create(capacity);

    if(!paginator->buffers)
    {
        MDV_LOGE("No memory for shared buffers cache");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_lrucache_release, paginator->buffers);

    if (mdv_mutex_create(&paginator->buffers_mutex) != MDV_OK)
    {
        MDV_LOGE("Mutex creation failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_mutex_free, &paginator->buffers_mutex);

    mdv_rollbacker_free(rollbacker);

    atomic_init(&paginator->rc, 1);

    paginator->page_size = page_size;

    return paginator;
}


mdv_paginator * mdv_paginator_retain(mdv_paginator *paginator)
{
    atomic_fetch_add_explicit(&paginator->rc, 1, memory_order_acquire);
    return paginator;
}


static void mdv_paginator_free(mdv_paginator *paginator)
{
    mdv_hashmap_release(paginator->storages);
    mdv_lrucache_release(paginator->buffers);
    mdv_mutex_free(&paginator->storages_mutex);
    mdv_mutex_free(&paginator->buffers_mutex);
    memset(paginator, 0, sizeof *paginator);
    mdv_free(paginator, "paginator");
}


uint32_t mdv_paginator_release(mdv_paginator *paginator)
{
    uint32_t rc = 0;

    if (paginator)
    {
        rc = atomic_fetch_sub_explicit(&paginator->rc, 1, memory_order_release) - 1;

        if (!rc)
            mdv_paginator_free(paginator);
    }

    return rc;
}
