/**
 * @file mdv_tables.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Tables storage
 * @version 0.1
 * @date 2019-11-15
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_def.h>
#include <mdv_table.h>
#include <mdv_rowset.h>


/// Tables storage
typedef struct mdv_tables mdv_tables;


/**
 * @brief Creates new or opens existing tables storage
 *
 * @param root_dir [in]     Root directory for tables storage
 *
 * @return tables storage
 */
mdv_tables * mdv_tables_open(char const *root_dir);


/**
 * @brief Add reference counter
 *
 * @param tables [in] Tables storage
 *
 * @return pointer which provided as argument
 */
mdv_tables * mdv_tables_retain(mdv_tables *tables);


/**
 * @brief Decrement references counter. Storage is closed and memory is freed if references counter is zero.
 *
 * @param tables [in] Tables storage
 *
 * @return references counter
 */
uint32_t mdv_tables_release(mdv_tables *tables);


/**
 * @brief Stores information about the new table.
 *
 * @param tables [in] Tables storage
 * @param uuid [in]   Table UUID
 * @param table [in]  Serialized table description (mdv_table)
 *
 * @return On success, return MDV_OK.
 * @return On error, return non zero value
 */
mdv_errno mdv_tables_add_raw(mdv_tables *tables, mdv_uuid const *uuid, mdv_data const *table);


/**
 * @brief Searches the table descriptor by UUID
 */
mdv_table * mdv_tables_get(mdv_tables *tables, mdv_uuid const *uuid);


/**
 * @brief Returns table descriptor for tables storage
 */
mdv_table * mdv_tables_desc(mdv_tables *tables);


/**
 * @brief Rows subset reading
 *
 * @param tables [in]    Tables storage
 * @param fields [in]    Fields mask for reading
 * @param count [in]     Rows amount for reading
 * @param rowid [out]    Last row identifier (used to continue reading)
 * @param filter [in]    Predicate for rowdata filtering
 * @param arg [in]       Argument which is passed to rowdata filtering predicate
 *
 * @return On success, returns nonzero pointer to rows set
 * @return On error, returns zero pointer
 */
mdv_rowset * mdv_tables_slice_from_begin(mdv_tables         *tables,
                                         mdv_bitset const   *fields,
                                         size_t              count,
                                         mdv_uuid           *rowid,
                                         mdv_row_filter      filter,
                                         void               *arg);


/**
 * @brief Rows subset reading from given row identifier
 *
 * @param tables [in]     Tables storage
 * @param fields [in]     Fields mask for reading
 * @param count [in]      Rows amount for reading
 * @param rowid [in][out] Last row identifier (used to continue reading)
 * @param filter [in]     Predicate for rowdata filtering
 * @param arg [in]        Argument which is passed to rowdata filtering predicate
 *
 * @return On success, returns nonzero pointer to rows set
 * @return On error, returns zero pointer
 */
mdv_rowset * mdv_tables_slice(mdv_tables        *tables,
                              mdv_bitset const  *fields,
                              size_t             count,
                              mdv_uuid          *rowid,
                              mdv_row_filter     filter,
                              void              *arg);

