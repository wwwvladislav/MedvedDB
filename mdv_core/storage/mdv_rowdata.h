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
#include <mdv_bitset.h>
#include <mdv_binn.h>


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
 * @return On success, returns MDV_OK.
 * @return On error, returns non zero value
 */
mdv_errno mdv_rowdata_add_raw(mdv_rowdata *rowdata, mdv_objid const *id, mdv_data const *row);


/**
 * @brief Stores rows set within one transaction
 *
 * @param rowdata [in] Rowdata storage
 * @param id [in]      First row identifier
 * @param row [in]     Serialized rows set
 *
 * @return On success, returns MDV_OK.
 * @return On error, returns non zero value
 */

mdv_errno mdv_rowdata_add_raw_rowset(mdv_rowdata *rowdata, mdv_objid const *id, binn *rowset);


/**
 * @brief Rows subset reading
 *
 * @param rowdata [in]   Rowdata storage
 * @param table [in]     Table descriptor
 * @param fields [in]    Fields mask for reading
 * @param count [in]     Rows amount for reading
 * @param rowid [out]    Last row identifier (used to continue reading)
 * @param filter [in]    Predicate for rowdata filtering
 * @param arg [in]       Argument which is passed to rowdata filtering predicate
 *
 * @return On success, returns nonzero pointer to rows set
 * @return On error, returns zero pointer
 */
mdv_rowset * mdv_rowdata_slice_from_begin(mdv_rowdata           *rowdata,
                                          mdv_table const       *table,
                                          mdv_bitset const      *fields,
                                          size_t                 count,
                                          mdv_objid             *rowid,
                                          mdv_row_filter         filter,
                                          void                  *arg);


/**
 * @brief Rows subset reading from given row identifier
 *
 * @param rowdata [in]    Rowdata storage
 * @param table [in]      Table descriptor
 * @param fields [in]     Fields mask for reading
 * @param count [in]      Rows amount for reading
 * @param rowid [in][out] Last row identifier (used to continue reading)
 * @param filter [in]     Predicate for rowdata filtering
 * @param arg [in]        Argument which is passed to rowdata filtering predicate
 *
 * @return On success, returns nonzero pointer to rows set
 * @return On error, returns zero pointer
 */
mdv_rowset * mdv_rowdata_slice(mdv_rowdata          *rowdata,
                               mdv_table const      *table,
                               mdv_bitset const     *fields,
                               size_t                count,
                               mdv_objid            *rowid,
                               mdv_row_filter        filter,
                               void                 *arg);

