/**
 * @brief Client types definitions.
 */
#pragma once
#include <mdv_channel.h>
#include <mdv_def.h>


/// Connection context types
extern const mdv_channel_t MDV_USER_CHANNEL;   /// User channel
extern const mdv_channel_t MDV_PEER_CHANNEL;   /// Peer channel


mdv_errno mdv_handshake_write(mdv_descriptor fd, mdv_channel_t type, mdv_uuid const *uuid);
mdv_errno mdv_handshake_read(mdv_descriptor fd, mdv_channel_t *type, mdv_uuid *uuid);
