/**
 * @file mdv_link.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Link state events definitions
 * @version 0.1
 * @date 2019-10-22
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_ebus.h>
#include <mdv_uuid.h>
#include <mdv_vector.h>


typedef struct
{
    mdv_event   base;
    mdv_uuid    from;       ///< Node UUID from which the event was received
    mdv_uuid    src;        ///< First node UUID
    mdv_uuid    dst;        ///< Second node UUID
    bool        connected;  ///< Link state
} mdv_evt_link_state;

mdv_evt_link_state * mdv_evt_link_state_create(
                                mdv_uuid const *from,
                                mdv_uuid const *src,
                                mdv_uuid const *dst,
                                bool            connected);
mdv_evt_link_state * mdv_evt_link_state_retain(mdv_evt_link_state *evt);
uint32_t             mdv_evt_link_state_release(mdv_evt_link_state *evt);


typedef struct
{
    mdv_event   base;
    mdv_vector *routes;     ///< Routes for broadcasting
    mdv_uuid    from;       ///< Node UUID from which the event was received
    mdv_uuid    src;        ///< First node UUID
    mdv_uuid    dst;        ///< Second node UUID
    bool        connected;  ///< Link state
} mdv_evt_link_state_broadcast;

mdv_evt_link_state_broadcast * mdv_evt_link_state_broadcast_create(
                                                mdv_vector     *routes,
                                                mdv_uuid const *from,
                                                mdv_uuid const *src,
                                                mdv_uuid const *dst,
                                                bool            connected);
mdv_evt_link_state_broadcast * mdv_evt_link_state_broadcast_retain(mdv_evt_link_state_broadcast *evt);
uint32_t                       mdv_evt_link_state_broadcast_release(mdv_evt_link_state_broadcast *evt);
