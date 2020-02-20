/**
 * @file mdv_row.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Single row storage
 * @version 0.1
 * @date 2020-02-20
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_data.h"


/// Row definition
typedef struct mdv_row
{
    mdv_data fields[1];
} mdv_row;
