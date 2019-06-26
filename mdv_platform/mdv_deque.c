#include "mdv_deque.h"
#include <stdint.h>


enum { MDV_DEQUE_BLOCK_SIZE = 1024 };   // items number in one block


struct mdv_deque
{
    size_t entry_size;      // item size
    size_t size;            // items count
    size_t offset;          // first item offset in first block
    size_t blocks_number;   // blocks number
    uint8_t **blocks;       // data blocks
};


mdv_deque * mdv_deque_create(size_t entry_size, size_t size)
{
}

