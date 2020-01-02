/**
 * @file mdv_rowdata.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Rowdata storage
 * @version 0.1
 * @date 2019-11-16
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_def.h>
#include <mdv_types.h>
#include <mdv_table.h>
#include <mdv_rowset.h>


/// Rowdata storage
typedef struct mdv_rowdata mdv_rowdata;


/**
 * @brief Creates new or opens existing rowdata storage
 *
 * @param dir [in]      directory for rowdata storage
 * @param table [in]    table identifier
 *
 * @return rowdata storage
 */
mdv_rowdata * mdv_rowdata_open(char const *dir, mdv_uuid const *table);


/**
 * @brief Add reference counter
 *
 * @param rowdata [in] Rowdata storage
 *
 * @return pointer which provided as argument
 */
mdv_rowdata * mdv_rowdata_retain(mdv_rowdata *rowdata);


/**
 * @brief Decrement references counter. Storage is closed and memory is freed if references counter is zero.
 *
 * @param rowdata [in] Rowdata storage
 *
 * @return references counter
 */
uint32_t mdv_rowdata_release(mdv_rowdata *rowdata);


/**
 * @brief Reserves identifiers range for rows
 *
 * @param rowdata [in]  Rowdata storage
 * @param range [in]    Identifiers range length
 * @param id [out]      first identifier from range
 *
 * @return On success, return MDV_OK.
 * @return On error, return non zero value
 */
mdv_errno mdv_rowdata_reserve(mdv_rowdata *rowdata, uint32_t range, uint64_t *id);


/**
 * @brief Stores table row.
 *
 * @param rowdata [in] Rowdata storage
 * @param id [in]      Row identifier
 * @param row [in]     Serialized rowdata (mdv_row_base)
 *
 * @return On success, return MDV_OK.
 * @return On error, return non zero value
 */
mdv_errno mdv_rowdata_add_raw(mdv_rowdata *rowdata, mdv_objid const *id, mdv_data const *row);


// mdv_rowset * mdv_rowdata_slice(mdv_rowdata *rowdata, mdv_objid const *id);
