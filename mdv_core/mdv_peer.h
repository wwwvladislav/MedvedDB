/**
 * @file
 * @brief Remote peers connections processing logic.
 * @details Remote peer might be a user or other cluster node.
 */
#pragma once
#include <mdv_uuid.h>
#include <mdv_def.h>
#include <mdv_chaman.h>
#include <mdv_msg.h>
#include <mdv_ebus.h>


/**
 * @brief Creates peer connection context
 *
 * @param fd [in]        channel descriptor
 * @param uuid [in]      current node uuid
 * @param peer_uuid [in] remote node uuid
 * @param dir [in]       channel direction
 * @param ebus [in]      events bus
 *
 * @return On success, return pointer to new peer context
 * @return On error, return NULL pointer
 */
mdv_channel * mdv_peer_create(mdv_descriptor  fd,
                              mdv_uuid const *uuid,
                              mdv_uuid const *peer_uuid,
                              mdv_channel_dir dir,
                              mdv_ebus       *ebus);
