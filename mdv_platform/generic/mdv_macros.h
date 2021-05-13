/**
 * @file mdv_macros.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Collection of useful macros
 * @version 0.1
 * @date 2021-04-24
 *
 * @copyright Copyright (c) 2021, Vladislav Volkov
 *
 */
#pragma once


#define mdv_sizeof_member(type, member) sizeof(((type *)0)->member)
