#include "mdv_buffer_manager.h"
#include <mdv_file.h>
#include <mdv_hashmap.h>
#include <mdv_limits.h>


typedef struct mdv_block
{
    uint8_t data[MDV_PAGE_SIZE];
} mdv_block;


struct mdv_buffer
{
    mdv_descriptor  fd;         ///< File associated with buffer
    mdv_hashmap    *blocks;     ///< Memory mapped blocks
};


struct mdv_buffer_manager
{
    size_t       pool_size;     ///< Memory pool size
    mdv_hashmap *buffers;       ///< Active buffers
};
