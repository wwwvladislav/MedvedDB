/**
 * @file mdv_evt_tables.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Tables storage events definitions
 * @version 0.1
 * @date 2020-01-15
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_ebus.h>
#include <mdv_types.h>
#include "../storage/mdv_tables.h"


typedef struct
{
    mdv_event        base;
    mdv_tables      *tables;     ///< Tables storage (out)
} mdv_evt_tables;

mdv_evt_tables * mdv_evt_tables_create();
mdv_evt_tables * mdv_evt_tables_retain(mdv_evt_tables *evt);
uint32_t         mdv_evt_tables_release(mdv_evt_tables *evt);
