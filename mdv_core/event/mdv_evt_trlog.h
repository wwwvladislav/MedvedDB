/**
 * @file mdv_evt_trlog.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Transaction logs events definitions
 * @version 0.1
 * @date 2019-11-12
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_ebus.h>
#include <mdv_uuid.h>


typedef struct
{
    mdv_event    base;
} mdv_evt_trlog_changed;

mdv_evt_trlog_changed * mdv_evt_trlog_changed_create();
mdv_evt_trlog_changed * mdv_evt_trlog_changed_retain(mdv_evt_trlog_changed *evt);
uint32_t                mdv_evt_trlog_changed_release(mdv_evt_trlog_changed *evt);


typedef struct
{
    mdv_event       base;
    mdv_uuid        uuid;       ///< Transaction log UUID
} mdv_evt_trlog_apply;

mdv_evt_trlog_apply * mdv_evt_trlog_apply_create(mdv_uuid const *uuid);
mdv_evt_trlog_apply * mdv_evt_trlog_apply_retain(mdv_evt_trlog_apply *evt);
uint32_t              mdv_evt_trlog_apply_release(mdv_evt_trlog_apply *evt);
