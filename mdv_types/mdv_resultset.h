/**
 * @file mdv_resultset.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief A table of data representing a database result set
 * @version 0.1
 * @date 2020-02-24
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_table.h"
#include <mdv_enumerator.h>


/// A table of data representing a database result set
typedef struct mdv_resultset mdv_resultset;


/**
 * @brief The result set constructor
 *
 * @param table [in]    Table descriptor
 * @param result [in]   Result set iterator
 *
 * @return pointer to the new result set
 */
mdv_resultset * mdv_resultset_create(mdv_table *table, mdv_enumerator *result);


/*
 * @brief Retains the result set.
 * @details Reference counter is increased by one.
 */
mdv_resultset * mdv_resultset_retain(mdv_resultset *resultset);


/**
 * @brief Releases the result set.
 * @details Reference counter is decreased by one.
 *          When the reference counter reaches zero, the result set's destructor is called.
 */
uint32_t mdv_resultset_release(mdv_resultset *resultset);


/**
 * @brief Returns the table descriptor
 */
mdv_table * mdv_resultset_table(mdv_resultset *resultset);


/**
 * @brief Returns result set iterator
 */
mdv_enumerator * mdv_resultset_enumerator(mdv_resultset *resultset);
