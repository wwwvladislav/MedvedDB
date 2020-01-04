/**
 * @file mdv_table.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Table description
 * @version 0.1
 * @date 2019-11-18
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_types.h"
#include <mdv_string.h>
#include <mdv_uuid.h>


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Table Description
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct mdv_table_desc
{
    mdv_string       name;      ///< Table name
    uint32_t         size;      ///< Fields count
    mdv_field const *fields;    ///< Fields
} mdv_table_desc;


/// Table descriptor
typedef struct mdv_table mdv_table;


/**
 * @brief Create new table desriptor
 *
 * @param id [in]   Table unique identifier
 * @param desc [in] Table description
 *
 * @return table desriptor or NULL
 */
mdv_table * mdv_table_create(mdv_uuid const *id, mdv_table_desc const *desc);


/**
 * @brief Retains table desriptor.
 * @details Reference counter is increased by one.
 */
mdv_table * mdv_table_retain(mdv_table *table);


/**
 * @brief Releases table desriptor.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the table desriptor is freed.
 */
uint32_t mdv_table_release(mdv_table *table);


/**
 * @brief Returns table unique identifier
 */
mdv_uuid const * mdv_table_uuid(mdv_table const *table);


/**
 * @brief Returns table description
 */
mdv_table_desc const * mdv_table_description(mdv_table const *table);
