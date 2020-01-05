/**
 * @file mdv_evt_view.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief View events definitions
 * @version 0.1
 * @date 2020-01-05
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_ebus.h>
#include <mdv_types.h>
#include <mdv_uuid.h>
#include <mdv_binn.h>
#include <mdv_bitset.h>


typedef struct
{
    mdv_event       base;
    mdv_uuid        session;    ///< Session identifier
    uint16_t        request_id; ///< Request identifier (used to associate requests and responses)
    mdv_uuid        table;      ///< Table identifier
    mdv_bitset     *fields;     ///< Fields mask
    char const     *filter;     ///< Predicate for rows filtering
} mdv_evt_select;

mdv_evt_select * mdv_evt_select_create(mdv_uuid const  *session,
                                       uint16_t         request_id,
                                       mdv_uuid const  *table,
                                       mdv_bitset      *fields,
                                       char const      *filter);
mdv_evt_select * mdv_evt_select_retain(mdv_evt_select *evt);
uint32_t         mdv_evt_select_release(mdv_evt_select *evt);


typedef struct
{
    mdv_event       base;
    mdv_uuid        session;    ///< Session identifier
    uint16_t        request_id; ///< Request identifier (used to associate requests and responses)
    uint32_t        view_id;    ///< View identifier
} mdv_evt_view;

mdv_evt_view * mdv_evt_view_create(mdv_uuid const  *session,
                                   uint16_t         request_id,
                                   uint32_t         view_id);
mdv_evt_view * mdv_evt_view_retain(mdv_evt_view *evt);
uint32_t       mdv_evt_view_release(mdv_evt_view *evt);


typedef struct
{
    mdv_event       base;
    mdv_uuid        session;    ///< Session identifier
    uint16_t        request_id; ///< Request identifier (used to associate requests and responses)
    uint32_t        view_id;    ///< View identifier
} mdv_evt_view_fetch;

mdv_evt_view_fetch * mdv_evt_view_fetch_create(mdv_uuid const  *session,
                                               uint16_t         request_id,
                                               uint32_t         view_id);
mdv_evt_view_fetch * mdv_evt_view_fetch_retain(mdv_evt_view_fetch *evt);
uint32_t             mdv_evt_view_fetch_release(mdv_evt_view_fetch *evt);
