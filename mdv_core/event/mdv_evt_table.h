/**
 * @file mdv_evt_table.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Table events definitions
 * @version 0.1
 * @date 2019-11-11
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_ebus.h>
#include <mdv_table.h>
#include <mdv_table.h>
#include <mdv_uuid.h>


typedef struct
{
    mdv_event        base;
    mdv_table_desc  *desc;      ///< Table description (in)
    mdv_uuid         table_id;  ///< Table identifier (out)
} mdv_evt_create_table;

mdv_evt_create_table * mdv_evt_create_table_create(mdv_table_desc **table);
mdv_evt_create_table * mdv_evt_create_table_retain(mdv_evt_create_table *evt);
uint32_t               mdv_evt_create_table_release(mdv_evt_create_table *evt);


typedef struct
{
    mdv_event        base;
    mdv_uuid         table_id;  ///< Table identifier (in)
    mdv_table       *table;     ///< Table descriptor (out)
} mdv_evt_table;

mdv_evt_table * mdv_evt_table_create(mdv_uuid const *table_id);
mdv_evt_table * mdv_evt_table_retain(mdv_evt_table *evt);
uint32_t        mdv_evt_table_release(mdv_evt_table *evt);
