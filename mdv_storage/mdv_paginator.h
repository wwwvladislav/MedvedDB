/**
 * @file mdv_paginator.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief The paginator (buffers manager) responsible for mapping disk blocks to main-memory.
 * @version 0.1
 * @date 2020-04-18
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_def.h>


/// Buffer identifier
typedef uint64_t mdv_buffer_id_t;


/// Buffers manager
typedef struct mdv_paginator mdv_paginator;


/// Buffer
typedef struct mdv_buffer mdv_buffer;


/**
 * @brief Create new buffers manager
 *
 * @param capacity [in]     maximum number of pages in memory
 * @param page_size [in]    page size (in bytes)
 * @param dir [in]          directory where buffer manager saves buffers
 *
 * @return On success, returns non zero pointer to new buffers manager
 * @return On error, return NULL pointer
 */
mdv_paginator * mdv_paginator_open(size_t capacity, size_t page_size, char const *dir);


/**
 * @brief Retains buffers manager.
 * @details Reference counter is increased by one.
 */
mdv_paginator * mdv_paginator_retain(mdv_paginator *paginator);


/**
 * @brief Releases buffers manager.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero,
 *          the buffers manager's destructor is called and all buffers are flushed.
 */
uint32_t mdv_paginator_release(mdv_paginator *paginator);


/**
 * @brief Allocates new buffer
 *
 * @param bm [in] buffers manager
 *
 * @return On success, returns non zero pointer to new buffer
 * @return On error, return NULL pointer
 */
//mdv_buffer * mdv_buffer_manager_new(mdv_buffer_manager *bm);


typedef enum
{
    MDV_BUF_READ    = 1 << 0,
    MDV_BUF_WRITE   = 1 << 1,
} mdv_buffer_mode;


/**
 * @brief Loads existing buffer with specified by identifier
 * @details If buffer loaded for writing it is marked as dirty.
 *
 * @param bm [in]   buffers manager
 * @param id [in]   buffer identifiers
 * @param mode [in] buffer loading mode
 *
 * @return On success, returns non zero pointer to buffer
 * @return On error, return NULL pointer
 */
//mdv_buffer * mdv_buffer_manager_get(mdv_buffer_manager *bm, mdv_buffer_id_t id, mdv_buffer_mode mode);


/**
 * @brief Forces a write of all buffer data to the disk.
 *
 * @param buffer [in]   memory mapped buffer
 *
 * @return On success, return MDV_OK
 * @return On error, return non zero value
 */
//mdv_errno mdv_buffer_flush(mdv_buffer *buffer);
