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
#include <mdv_types.h>
#include <mdv_rowset.h>
#include <mdv_uuid.h>
#include <mdv_binn.h>
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
    mdv_uuid        session;    ///< Session identifier
    uint16_t        request_id; ///< Request identifier (used to associate requests and responses)
    mdv_uuid        table;      ///< Table identifier
    bool            first;      ///< Flag indicates that the first row should be fetched
    mdv_objid       rowid;      ///< First row identifier to be fetched
    uint32_t        count;      ///< Batch size to be fetched
} mdv_evt_rowdata_fetch;

mdv_evt_rowdata_fetch * mdv_evt_rowdata_fetch_create(mdv_uuid const  *session,
                                                     uint16_t         request_id,
                                                     mdv_uuid const  *table,
                                                     bool             first,
                                                     mdv_objid const *rowid,
                                                     uint32_t         count);
mdv_evt_rowdata_fetch * mdv_evt_rowdata_fetch_retain(mdv_evt_rowdata_fetch *evt);
uint32_t                mdv_evt_rowdata_fetch_release(mdv_evt_rowdata_fetch *evt);


typedef struct
{
    mdv_event       base;
    mdv_uuid        table;      ///< Table identifier (in)
    mdv_rowdata    *rowdata;    ///< Rowdata storage (out)
} mdv_evt_rowdata;

mdv_evt_rowdata * mdv_evt_rowdata_create(mdv_uuid const *table);
mdv_evt_rowdata * mdv_evt_rowdata_retain(mdv_evt_rowdata *evt);
uint32_t          mdv_evt_rowdata_release(mdv_evt_rowdata *evt);
