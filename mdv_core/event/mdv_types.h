/**
 * @file mdv_types.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Internal event types identifiers
 * @version 0.1
 * @date 2019-10-21
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once


enum
{
    MDV_EVT_BROADCAST,
    MDV_EVT_LINK_STATE,
    MDV_EVT_LINK_STATE_BROADCAST,
    MDV_EVT_TOPOLOGY,
    MDV_EVT_TOPOLOGY_SYNC,
    MDV_EVT_ROUTES_CHANGED,
    MDV_EVT_COUNT
};

