/**
 * @file mdv_evt_link.h
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
#include <mdv_hashmap.h>
#include <mdv_topology.h>


typedef struct
{
    mdv_event       base;
    mdv_uuid        from;       ///< Node UUID from which the event was received
    mdv_toponode    src;        ///< First node
    mdv_toponode    dst;        ///< Second node
    bool            connected;  ///< Link state
} mdv_evt_link_state;

mdv_evt_link_state * mdv_evt_link_state_create(
                                mdv_uuid const     *from,
                                mdv_toponode const *src,
                                mdv_toponode const *dst,
                                bool                connected);
mdv_evt_link_state * mdv_evt_link_state_retain(mdv_evt_link_state *evt);
uint32_t             mdv_evt_link_state_release(mdv_evt_link_state *evt);

