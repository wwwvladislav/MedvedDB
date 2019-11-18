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
    mdv_string       name;
    mdv_uuid         id;
    uint32_t         size;
    mdv_field const *fields;
} mdv_table_desc;


mdv_table_desc * mdv_table_desc_clone(mdv_table_desc const *table);


/// Table descriptor
typedef struct mdv_table mdv_table;


mdv_table * mdv_table_create(mdv_uuid const *id, mdv_table_desc const *desc);
mdv_table *mdv_table_retain(mdv_table *table);
uint32_t mdv_table_release(mdv_table *table);
