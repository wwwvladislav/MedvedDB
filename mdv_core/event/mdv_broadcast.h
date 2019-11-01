/**
 * @file mdv_broadcast.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Event definition for messages broadcasting
 * @version 0.1
 * @date 2019-10-30
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_ebus.h>
#include <mdv_hashmap.h>
#include <mdv_topology.h>
#include <mdv_uuid.h>


typedef struct
{
    mdv_event       base;
    uint16_t        msg_id;             ///< Message identifier
    uint32_t        size;               ///< Data size for broadcasing
    void           *data;               ///< Data for broadcasing
    mdv_hashmap    *notified;           ///< Notified nodes UUIDs
    mdv_hashmap    *peers;              ///< Neighbour UUIDs for broadcasting
} mdv_evt_broadcast_post;

mdv_evt_broadcast_post * mdv_evt_broadcast_post_create(
                                uint16_t        msg_id,
                                uint32_t        size,
                                void           *data,
                                mdv_hashmap    *notified,
                                mdv_hashmap    *peers);
mdv_evt_broadcast_post * mdv_evt_broadcast_post_retain(mdv_evt_broadcast_post *evt);
uint32_t                 mdv_evt_broadcast_post_release(mdv_evt_broadcast_post *evt);


typedef struct
{
    mdv_event       base;
    mdv_uuid        from;               ///< Node UUID from which the event was received
    uint16_t        msg_id;             ///< Message identifier
    uint32_t        size;               ///< Data size for broadcasing
    void           *data;               ///< Data for broadcasing
    mdv_hashmap    *notified;           ///< Notified nodes UUIDs
} mdv_evt_broadcast;


mdv_evt_broadcast * mdv_evt_broadcast_create(
                        mdv_uuid const *from,
                        uint16_t        msg_id,
                        uint32_t        size,
                        void           *data,
                        mdv_hashmap    *notified);
mdv_evt_broadcast * mdv_evt_broadcast_retain(mdv_evt_broadcast *evt);
uint32_t            mdv_evt_broadcast_release(mdv_evt_broadcast *evt);
