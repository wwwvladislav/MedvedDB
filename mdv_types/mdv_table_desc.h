/**
 * @file mdv_table_desc.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Table description
 * @version 0.1
 * @date 2020-02-17
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_field.h"


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Table Description
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct mdv_table_desc
{
    char const      *name;      ///< Table name
    uint32_t         size;      ///< Fields count
    mdv_field const *fields;    ///< Fields
} mdv_table_desc;
