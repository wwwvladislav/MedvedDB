/**
 * @file mdv_evt_rowdata.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Row data events definitions
 * @version 0.1
 * @date 2019-11-22
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_ebus.h>
#include <mdv_rowset.h>
#include <mdv_uuid.h>
#include <mdv_binn.h>
#include <mdv_bitset.h>
#include "../storage/mdv_rowdata.h"


typedef struct
{
    mdv_event       base;
    mdv_uuid        table_id;   ///< Table identifier
    binn           *rows;       ///< Serialized rows for insert
} mdv_evt_rowdata_ins_req;

mdv_evt_rowdata_ins_req * mdv_evt_rowdata_ins_req_create(mdv_uuid const *table_id, binn *rows);
mdv_evt_rowdata_ins_req * mdv_evt_rowdata_ins_req_retain(mdv_evt_rowdata_ins_req *evt);
uint32_t                  mdv_evt_rowdata_ins_req_release(mdv_evt_rowdata_ins_req *evt);


typedef struct
{
    mdv_event       base;
    mdv_uuid        table;      ///< Table identifier (in)
    mdv_rowdata    *rowdata;    ///< Rowdata storage (out)
} mdv_evt_rowdata;

mdv_evt_rowdata * mdv_evt_rowdata_create(mdv_uuid const *table);
mdv_evt_rowdata * mdv_evt_rowdata_retain(mdv_evt_rowdata *evt);
uint32_t          mdv_evt_rowdata_release(mdv_evt_rowdata *evt);
