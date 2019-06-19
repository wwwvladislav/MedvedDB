#pragma once
#include "mdv_stack.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>


typedef struct
{
    uint32_t    size;
    char       *ptr;
} mdv_string;


#define mdv_str_empty(str)          ((str).size < 2 || !(str).ptr)
#define mdv_str_static(str)         (mdv_string){ sizeof(str), str }
#define mdv_str(str)                (mdv_string){ strlen(str) + 1, str }
#define mdv_str_null                (mdv_string){ 0, 0 }


#define mdv_str_pdup(stack, str)                                \
    (mdv_string){ strlen(str) + 1, mdv_stack_push(stack, str, strlen(str) + 1) }


#define mdv_str_len_1(str) ((str).size ? (str).size - 1 : 0)
#define mdv_str_len_2(str1, str2) (mdv_str_len_1(str1) + mdv_str_len_1(str2))
#define mdv_str_len_3(str1, str2, str3) (mdv_str_len_2(str1, str2) + mdv_str_len_1(str3))
#define mdv_str_len_4(str1, str2, str3, str4) (mdv_str_len_3(str1, str2, str3) + mdv_str_len_1(str4))
#define mdv_str_len_5(str1, str2, str3, str4, str5) (mdv_str_len_4(str1, str2, str3, str4) + mdv_str_len_1(str5))

#define mdv_str_op_N(_1, _2, _3, _4, _5, M, ...) M

#define mdv_str_expand_1(str) str.ptr, str.size - 1
#define mdv_str_expand_2(str1, str2) mdv_str_expand_1(str1), mdv_str_expand_1(str2)
#define mdv_str_expand_3(str1, str2, str3) mdv_str_expand_2(str1, str2), mdv_str_expand_1(str3)
#define mdv_str_expand_4(str1, str2, str3, str4) mdv_str_expand_3(str1, str2, str3), mdv_str_expand_1(str4)
#define mdv_str_expand_5(str1, str2, str3, str4, str5) mdv_str_expand_4(str1, str2, str3, str4), mdv_str_expand_1(str5)

#define mdv_str_len(...)                                        \
    mdv_str_op_N(__VA_ARGS__,                                   \
                  mdv_str_len_5(__VA_ARGS__),                   \
                  mdv_str_len_4(__VA_ARGS__),                   \
                  mdv_str_len_3(__VA_ARGS__),                   \
                  mdv_str_len_2(__VA_ARGS__),                   \
                  mdv_str_len_1(__VA_ARGS__))

#define mdv_str_expand(...)                                     \
    mdv_str_op_N(__VA_ARGS__,                                   \
                  mdv_str_expand_5(__VA_ARGS__),                \
                  mdv_str_expand_4(__VA_ARGS__),                \
                  mdv_str_expand_3(__VA_ARGS__),                \
                  mdv_str_expand_2(__VA_ARGS__),                \
                  mdv_str_expand_1(__VA_ARGS__))

#define mdv_str_pcat(stack, dst, ...)                                       \
    mdv_stack_free_space(stack) < mdv_str_len(__VA_ARGS__)                  \
    || mdv_stack_top(stack) + 1 != dst.ptr + dst.size                       \
        ? (mdv_string)mdv_str_null                                          \
        : (mdv_stack_pop(stack),                                            \
           mdv_stack_push_all(stack, mdv_str_expand(__VA_ARGS__)),          \
           mdv_stack_push(stack, (char)0),                                  \
           (mdv_string) { dst.size + mdv_str_len(__VA_ARGS__), dst.ptr })
