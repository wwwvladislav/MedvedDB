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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


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


static const char MDV_PAGEFILE[] = "pagefile";


static size_t mdv_size_hash(size_t const *v)                { return *v; }
static int mdv_size_cmp(size_t const *a, size_t const *b)   { return (int)*a - *b; }


/// Pages storage
typedef struct mdv_pages_storage
{
    atomic_uint_fast32_t rc;                        ///< References counter
    uint32_t             id;                        ///< Pages storage identifier
    mdv_descriptor       fd;                        ///< File associated with page
    size_t               used;                      ///< Used pages count
    size_t               total;                     ///< Total pages count
} mdv_pages_storage;


static char const * mdv_pages_storage_path(
    char *path,
    size_t size,
    char const *dir,
    char const *name,
    uint32_t id)
{
    snprintf(path, size, "%s/%s-%u", dir, name, id);
    return path;
}


static mdv_errno mdv_pages_storage_open(
    char const *dir,
    char const *name,
    uint32_t id,
    mdv_pages_storage **pstorage)
{
    mdv_rollbacker *rollbacker = mdv_rollbacker_create(2);

    char tmp[MDV_PATH_MAX];
    char const *path = mdv_pages_storage_path(tmp, sizeof tmp, dir, name, id);

    mdv_descriptor fd = mdv_open(path, MDV_OCREAT | MDV_OREAD | MDV_OWRITE | MDV_ODIRECT | MDV_ODSYNC);

    if (fd == MDV_INVALID_DESCRIPTOR)
    {
        MDV_LOGE("Pages storage '%s' opening failed", path);
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    mdv_rollbacker_push(rollbacker, mdv_descriptor_close, fd);

    mdv_pages_storage *storage = mdv_alloc(sizeof(mdv_pages_storage), "pages_storage");

    if (!storage)
    {
        MDV_LOGE("No memory for new pages storage");
        mdv_rollback(rollbacker);
        return MDV_NO_MEM;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, storage, "pages_storage");

    atomic_init(&storage->rc, 1);

    storage->id = id;
    storage->fd = fd;
    storage->used = 0;
    storage->total = 0;

    size_t file_size = 0;

    mdv_errno err = mdv_file_size_by_fd(fd, &file_size);

    if (err != MDV_OK)
    {
        MDV_LOGE("Pages file size determination failed with error '%s' (%d)",
                    mdv_strerror(err, tmp, sizeof tmp), err);
        mdv_rollback(rollbacker);
        return MDV_FAILED;
    }

    if (!file_size)
    {
        // TODO: mdv_pages_storage_init
        int n = 0;
        ++n;
    }
    else
    {
        // TODO: mdv_pages_storage_load
        int n = 0;
        ++n;
    }

    mdv_rollbacker_free(rollbacker);

    *pstorage = storage;

    return MDV_OK;
}


static void mdv_pages_storage_close(mdv_pages_storage *storage)
{
    mdv_descriptor_close(storage->fd);
    storage->fd = MDV_INVALID_DESCRIPTOR;
}


/// Buffer
struct mdv_buffer
{
    mdv_pageid           id;                        ///< First page identifier
    mdv_pages_storage   *storage;                   ///< Buffer storage
    uint16_t             pinned:1;                  ///< buffer cannot at the moment be written back to disk safely (in use)
    uint16_t             dirty:1;                   ///< buffer has been changed
    uint32_t             size;                      ///< Data size
    uint8_t              data[1];                   ///< Data
};


/// The page header stored in the file
typedef struct mdv_page_hdr
{
    uint32_t             size;                      ///< Data size (in bytes)
} mdv_page_hdr;


/// Free pages list
typedef struct
{
    size_t      size;                               ///< Pages cluster size
    mdv_vector *identifiers;                        ///< First page identifiers of free clusters
} mdv_free_pages;


struct mdv_paginator
{
    atomic_uint_fast32_t rc;                        ///< References counter
    mdv_hashmap         *free_pages;                ///< Free page clusters (hashmap<size, mdv_free_pages>)
    mdv_hashmap         *storages;                  ///< Active storages (hashmap<fd, mdv_pages_storage>)
    mdv_mutex            storages_mutex;            ///< Mutex for storages guard
    mdv_lrucache        *buffers;                   ///< Memory mapped buffers
    mdv_mutex            buffers_mutex;             ///< Mutex for buffers guard
    size_t               page_size;                 ///< Page size
};


static void mdv_paginator_storages_free(mdv_hashmap *storages)
{
    mdv_hashmap_foreach(storages, mdv_pages_storage, entry)
        mdv_pages_storage_close(entry);
    mdv_hashmap_release(storages);
}


static void mdv_paginator_free_pages_free(mdv_hashmap *free_pages)
{
    mdv_hashmap_foreach(free_pages, mdv_free_pages, entry)
    {
        mdv_vector_release(entry->identifiers);
        entry->identifiers = 0;
    }

    mdv_hashmap_release(free_pages);
}


static mdv_hashmap * mdv_paginator_storages_open(char const *dir, char const *name)
{
    mdv_hashmap *storages = mdv_hashmap_create(mdv_pages_storage,
                                             fd,
                                             4,
                                             mdv_descriptor_hash,
                                             mdv_descriptor_cmp);

    if (!storages)
    {
        MDV_LOGE("No memory for new buffers storages map");
        return 0;
    }

    mdv_enumerator *dir_enumerator = mdv_dir_enumerator(dir);

    if (!dir_enumerator)
    {
        MDV_LOGE("Unable to read directory '%s'", dir);
        mdv_hashmap_release(storages);
        return 0;
    }

    while(mdv_enumerator_next(dir_enumerator) == MDV_OK)
    {
        char const *file = mdv_enumerator_current(dir_enumerator);
        const size_t name_len = strlen(name);

        if (strcmp(file, name) == 0
            && file[sizeof(MDV_PAGEFILE) - 1] == '-')
        {
            long const id = atol(file + name_len + 1);

            mdv_pages_storage *storage = 0;

            if (mdv_pages_storage_open(dir, name, (uint32_t)id, &storage) != MDV_OK)
            {
                mdv_paginator_storages_free(storages);
                storages = 0;
                break;
            }

            if (!mdv_hashmap_insert(storages, storage, sizeof *storage))
            {
                MDV_LOGE("No memory for page file");
                mdv_paginator_storages_free(storages);
                storages = 0;
                break;
            }
        }
    }

    mdv_enumerator_release(dir_enumerator);

    return storages;
}


mdv_paginator * mdv_paginator_open(size_t capacity, size_t page_size, char const *dir)
{
    // Create buffers persist directory
    if (!mdv_mkdir(dir))
    {
        MDV_LOGE("Buffers persist directory creation failed: '%s'", dir);
        return 0;
    }

    mdv_rollbacker *rollbacker = mdv_rollbacker_create(6);

    mdv_paginator *paginator = mdv_alloc(sizeof(mdv_paginator), "paginator");

    if (!paginator)
    {
        MDV_LOGE("No memory for new buffers manager");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_free, paginator, "paginator");

    paginator->free_pages = mdv_hashmap_create(mdv_free_pages, size, 4, mdv_size_hash, mdv_size_cmp);

    if (!paginator->free_pages)
    {
        MDV_LOGE("No memory for free pages map");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_paginator_free_pages_free, paginator->free_pages);

    paginator->storages = mdv_paginator_storages_open(dir, MDV_PAGEFILE);

    if (!paginator->storages)
    {
        MDV_LOGE("Page files reading failed");
        mdv_rollback(rollbacker);
        return 0;
    }

    mdv_rollbacker_push(rollbacker, mdv_paginator_storages_free, paginator->storages);

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
    mdv_paginator_free_pages_free(paginator->free_pages);
    mdv_paginator_storages_free(paginator->storages);
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


mdv_buffer * mdv_paginator_allocate(mdv_paginator *paginator, size_t size)
{
    return 0;
}

