/**
 * @file mdv_routes.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Routes events definitions
 * @version 0.1
 * @date 2019-10-22
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_ebus.h>
#include <mdv_hashmap.h>


typedef struct
{
    mdv_event    base;
    mdv_hashmap *routes;
} mdv_evt_routes_changed;

mdv_evt_routes_changed * mdv_evt_routes_changed_create(mdv_hashmap *routes);
mdv_evt_routes_changed * mdv_evt_routes_changed_retain(mdv_evt_routes_changed *evt);
uint32_t                 mdv_evt_routes_changed_release(mdv_evt_routes_changed *evt);
