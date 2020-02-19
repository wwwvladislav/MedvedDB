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
    char const      *name;          ///< Table name
    uint32_t         size;          ///< Fields count
    bool             dynamic_alloc; ///< Flag indicates dynamic allocation
    mdv_field const *fields;        ///< Fields
} mdv_table_desc;


/**
 * @brief Constructor for table description
 */
mdv_table_desc * mdv_table_desc_create(char const *name);


/**
 * @brief Destructor for table description
 */
void mdv_table_desc_free(mdv_table_desc *desc);


/**
 * @brief Appends new fields to the end of fields array
  */
bool mdv_table_desc_append(mdv_table_desc *desc, mdv_field const *field);
