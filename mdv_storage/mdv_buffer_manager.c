#include "mdv_buffer_manager.h"
#include <mdv_file.h>
#include <mdv_hashmap.h>
#include <mdv_lrucache.h>
#include <mdv_limits.h>
#include <mdv_assert.h>
#include <mdv_mutex.h>
#include <mdv_vector.h>
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
        mdv_buffer_metainf header;              ///< Buffer metai nformation
        uint8_t            ptr[1];              ///< Data pointer
    } data;                                     ///< Data
};


mdv_static_assert(sizeof(mdv_buffer_runtime) == 16u);
mdv_static_assert(sizeof(mdv_buffer_metainf) == 16u);
mdv_static_assert(offsetof(mdv_buffer, data.header) == 16u);
mdv_static_assert(offsetof(mdv_buffer, data.ptr) == 32u);


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
