/**
 * @file mdv_node.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Node events definitions
 * @version 0.1
 * @date 2019-10-21
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_ebus.h>
#include "../mdv_node.h"


typedef struct
{
    mdv_event   base;
    mdv_node    node;
} mdv_evt_node_up;

mdv_evt_node_up * mdv_evt_node_up_create(mdv_uuid const *uuid, char const *addr);
mdv_evt_node_up * mdv_evt_node_up_retain(mdv_evt_node_up *evt);
uint32_t          mdv_evt_node_up_release(mdv_evt_node_up *evt);


typedef struct
{
    mdv_event   base;
    mdv_uuid    uuid;
} mdv_evt_node_down;

mdv_evt_node_down * mdv_evt_node_down_create(mdv_uuid const *uuid);
mdv_evt_node_down * mdv_evt_node_down_retain(mdv_evt_node_down *evt);
uint32_t            mdv_evt_node_down_release(mdv_evt_node_down *evt);


typedef struct
{
    mdv_event   base;
    mdv_node    node;
} mdv_evt_node_registered;

mdv_evt_node_registered * mdv_evt_node_registered_create(mdv_node const *node);
mdv_evt_node_registered * mdv_evt_node_registered_retain(mdv_evt_node_registered *evt);
uint32_t                  mdv_evt_node_registered_release(mdv_evt_node_registered *evt);

