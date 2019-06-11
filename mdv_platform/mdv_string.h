#pragma once
#include "mdv_stack.h"
#include <stddef.h>
#include <string.h>


typedef struct
{
    size_t  size;
    char   *ptr;
} mdv_string;


#define mdv_str_empty(str)          ((str).size < 2 || !(str).ptr)
#define mdv_str_static(str)         { sizeof(str), str }
#define mdv_str(str)                { strlen(str) + 1, str }
#define mdv_str_null                { 0, 0 }


#define mdv_str_pdup(stack, str)                                \
    (mdv_string){ strlen(str) + 1, mdv_stack_push(stack, str, strlen(str) + 1) }


#define mdv_str_pcat(stack, dst, str)                           \
    mdv_stack_free_space(stack) < str.size - 1                  \
        ? (mdv_string)mdv_str_null                              \
        : (mdv_stack_pop(stack),                                \
           mdv_stack_push(stack, str.ptr, str.size - 1),        \
           mdv_stack_push(stack, (char)0),                      \
           (mdv_string) { dst.size + str.size - 1, dst.ptr })
