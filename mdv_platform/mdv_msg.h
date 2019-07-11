#pragma once
#include "mdv_def.h"
#include <stdint.h>


/// Message flags
enum mdv_msg_flags
{
    MDV_MSG_DYNALLOC = 1 << 0   ///< payload buffer allocated dynamically. It needs to be freed with mdv_free().
};


/// Message header description
typedef struct mdv_msghdr
{
    uint16_t    id;             ///< Message identifier
    uint16_t    number;         ///< Message number (used to associate requests and responses)
    uint32_t    size;           ///< Message payload size
} mdv_msghdr;


typedef struct mdv_msg
{
    mdv_msghdr  hdr;            ///< Message header
    uint32_t    flags;          ///< Flags (see mdv_msg_flags)
    uint32_t    available_size; ///< Available data size in header + payload
    void       *payload;        ///< Pointer to the message payload
} mdv_msg;


/**
 * @brief Function for writing message to a file descriptor.
 *
 * @param fd [in]   file descriptor
 * @param msg [in]  message to be send
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_write_msg(mdv_descriptor fd, mdv_msg const *msg);


/**
 * @brief Function for reading message from a file descriptor.
 *
 * @param fd [in]   file descriptor
 * @param msg [in]  message to be read \n
 *            [out] number of bytes which has been read is written to available_size field
 *
 * @return MDV_OK on success
 * @return nonzero value on error
 */
mdv_errno mdv_read_msg(mdv_descriptor fd, mdv_msg *msg);


/**
 * @brief Clear and free message
 *
 * @param msg [in] message
 */
void mdv_free_msg(mdv_msg *msg);

