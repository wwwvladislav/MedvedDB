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
#include <mdv_types.h>


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
 * @param table [in]  Serialized table description (mdv_table_base)
 *
 * @return On success, return MDV_OK.
 * @return On error, return non zero value
 */
mdv_errno mdv_tables_add_raw(mdv_tables *tables, mdv_uuid const *uuid, mdv_data const *table);
