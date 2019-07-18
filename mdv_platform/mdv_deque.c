#include "mdv_deque.h"
#include "mdv_alloc.h"
#include <stdint.h>
#include <string.h>


enum
{
    MDV_DEQUE_BLOCK_SIZE        = 1024,     // items in one block
    MDV_DEQUE_BLOCKS_INC_SIZE   = 256       // blocks number grow size
};


struct mdv_deque
{
    size_t entry_size;      // item size
    size_t size;            // items count
    size_t offset;          // first item offset in first block
    size_t blocks_number;   // blocks number
    size_t allocated;       // total number of allocated bytes
    uint8_t **blocks;       // data blocks
};


mdv_deque * mdv_deque_create(size_t entry_size)
{
    mdv_deque *deque = (mdv_deque*)mdv_alloc(sizeof(mdv_deque), "deque");

    if (deque)
    {
        deque->entry_size = entry_size;
        deque->size = 0;
        deque->offset = 0;
        deque->blocks_number = MDV_DEQUE_BLOCKS_INC_SIZE;

        deque->blocks = (uint8_t **)mdv_alloc(sizeof(uint8_t*) * deque->blocks_number, "deque.blocks");

        if (!deque->blocks)
        {
            mdv_free(deque, "deque");
            return 0;
        }

        memset(deque->blocks, 0, sizeof(uint8_t*) * deque->blocks_number);

        deque->allocated = sizeof(uint8_t*) * deque->blocks_number;
    }

    return deque;
}


void mdv_deque_free(mdv_deque *deque)
{
    if (deque)
    {
        if (deque->blocks)
        {
            for(size_t i = 0;
                i < deque->blocks_number
                    && deque->blocks[i];
                ++i)
            {
                mdv_free(deque->blocks[i], "deque.blocks[i]");
                deque->allocated -= deque->entry_size * MDV_DEQUE_BLOCK_SIZE;
            }
            mdv_free(deque->blocks, "deque.blocks");
            deque->allocated -= sizeof(uint8_t*) * deque->blocks_number;
        }
        mdv_free(deque, "deque");
    }
}


bool mdv_deque_push_back(mdv_deque *deque, void const *data, size_t data_size)
{
    if (data_size != deque->entry_size)
        return false;

    size_t const size           = deque->size + deque->offset;
    size_t const filled_blocks  = (size + MDV_DEQUE_BLOCK_SIZE - 1) / MDV_DEQUE_BLOCK_SIZE;
    size_t const free_space     = filled_blocks * MDV_DEQUE_BLOCK_SIZE - size;

    if (free_space)
    {
        uint8_t *block = deque->blocks[filled_blocks - 1];
        memcpy(block + (MDV_DEQUE_BLOCK_SIZE - free_space) * deque->entry_size, data, data_size);
        deque->size++;
    }
    else
    {
        uint8_t *block = 0;

        if (filled_blocks < deque->blocks_number)
        {
            block = deque->blocks[filled_blocks];
            if (!block)
            {
                block = deque->blocks[filled_blocks] = (uint8_t*)mdv_alloc(deque->entry_size * MDV_DEQUE_BLOCK_SIZE, "deque.blocks[i]");
                if (!block)
                    return false;
                deque->allocated += deque->entry_size * MDV_DEQUE_BLOCK_SIZE;
            }
        }
        else    // no space for new block
        {
            uint8_t **blocks = (uint8_t **)mdv_realloc(deque->blocks, sizeof(uint8_t*) * (deque->blocks_number + MDV_DEQUE_BLOCKS_INC_SIZE), "deque.blocks");
            if (!blocks)
                return false;
            memset(blocks + deque->blocks_number, 0, sizeof(uint8_t*) * MDV_DEQUE_BLOCKS_INC_SIZE);
            deque->blocks = blocks;
            deque->blocks_number += MDV_DEQUE_BLOCKS_INC_SIZE;
            deque->allocated += sizeof(uint8_t*) * MDV_DEQUE_BLOCKS_INC_SIZE;

            // Allocate new block
            block = deque->blocks[filled_blocks] = (uint8_t*)mdv_alloc(deque->entry_size * MDV_DEQUE_BLOCK_SIZE, "deque.blocks[i]");
            if (!block)
                return false;
            deque->allocated += deque->entry_size * MDV_DEQUE_BLOCK_SIZE;
        }

        memcpy(block, data, data_size);
        deque->size++;
    }

    return true;
}


bool mdv_deque_pop_back(mdv_deque *deque, void *e)
{
    if (!deque->size)
        return false;

    size_t const size           = deque->size + deque->offset;
    size_t const filled_blocks  = (size + MDV_DEQUE_BLOCK_SIZE - 1) / MDV_DEQUE_BLOCK_SIZE;

    uint8_t *block              = deque->blocks[filled_blocks - 1];
    size_t const offset         = (size + MDV_DEQUE_BLOCK_SIZE - 1) % MDV_DEQUE_BLOCK_SIZE;

    memcpy(e, block + deque->entry_size * offset, deque->entry_size);
    deque->size--;

    if (!offset)    // block is emptry
    {
        mdv_free(block, "deque.blocks[i]");
        deque->blocks[filled_blocks - 1] = 0;
        deque->allocated -= deque->entry_size * MDV_DEQUE_BLOCK_SIZE;
    }

    return true;
}

