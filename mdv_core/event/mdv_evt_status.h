/**
 * @file mdv_evt_status.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Status events definition
 * @version 0.1
 * @date 2019-12-16
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
    mdv_uuid        session;    ///< Session identifier
    uint16_t        request_id; ///< Request identifier (used to associate requests and responses)
    int             err;        ///< Error core
    char const     *message;    ///< Error message
} mdv_evt_status;

mdv_evt_status * mdv_evt_status_create(
                                mdv_uuid const *session,
                                uint16_t        request_id,
                                int             err,
                                char const     *message);
mdv_evt_status * mdv_evt_status_retain(mdv_evt_status *evt);
uint32_t         mdv_evt_status_release(mdv_evt_status *evt);
