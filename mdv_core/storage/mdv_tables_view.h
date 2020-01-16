/**
 * @file mdv_tables_view.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief View implemention for rowdata storage
 * @version 0.1
 * @date 2020-01-16
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_view.h"
#include "mdv_predicate.h"
#include "mdv_tables.h"


/**
 * @brief Creates new view
 */
mdv_view * mdv_tables_view_create(mdv_tables    *source,
                                  mdv_bitset    *fields,
                                  mdv_predicate *predicate);
