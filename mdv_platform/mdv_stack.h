#pragma once
#include "mdv_numeric.h"
#include <string.h>


typedef struct
{
    size_t  capacity;
    size_t  size;
    char    data[1];
} mdv_stack_base;


#define mdv_stack(type, sz)     \
    struct                      \
    {                           \
        size_t  capacity;       \
        size_t  size;           \
        type    data[sz];       \
    }


#define mdv_stack_clear(st)                                 \
    (st).capacity = sizeof((st).data) / sizeof(*(st).data); \
    (st).size = 0;


#define mdv_stack_push_multiple(st, items, count)                                                       \
    ((st).size + (count) <= (st).capacity && sizeof(*(items)) == sizeof(*(st).data)                     \
        ? memcpy((st).data + mdv_postfix_add((st).size, (count)), (items), sizeof(*(items)) * (count)), \
          (st).data + (st).size - (count)                                                               \
        : 0)


#define mdv_stack_push_one(st, item)                                    \
    ((st).size < (st).capacity && sizeof(item) == sizeof(*(st).data)    \
        ? (st).data[(st).size++] = (item), (st).data + (st).size - 1    \
        : 0)


#define mdv_stack_push_any(st, items, count, M, ...) M
#define mdv_stack_push(...)                                     \
    mdv_stack_push_any(__VA_ARGS__,                             \
                       mdv_stack_push_multiple(__VA_ARGS__),    \
                       mdv_stack_push_one(__VA_ARGS__))


#define mdv_stack_push_1(st, item1, count1) mdv_stack_push(st, item1, count1)
#define mdv_stack_push_2(st, item1, count1, item2, count2) (mdv_stack_push_1(st, item1, count1), mdv_stack_push(st, item2, count2))
#define mdv_stack_push_3(st, item1, count1, item2, count2, item3, count3) (mdv_stack_push_2(st, item1, count1, item2, count2), mdv_stack_push(st, item3, count3))
#define mdv_stack_push_4(st, item1, count1, item2, count2, item3, count3, item4, count4) (mdv_stack_push_3(st, item1, count1, item2, count2, item3, count3), mdv_stack_push(st, item4, count4))
#define mdv_stack_push_5(st, item1, count1, item2, count2, item3, count3, item4, count4, item5, count5) (mdv_stack_push_4(st, item1, count1, item2, count2, item3, count3, item4, count4), mdv_stack_push(st, item5, count5))
#define mdv_stack_push_N(st, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define mdv_stack_push_all(st, ...)                             \
    mdv_stack_push_N(st, __VA_ARGS__,                           \
                     mdv_stack_push_5(st, __VA_ARGS__),         \
                     mdv_stack_push_5(st, __VA_ARGS__),         \
                     mdv_stack_push_4(st, __VA_ARGS__),         \
                     mdv_stack_push_4(st, __VA_ARGS__),         \
                     mdv_stack_push_3(st, __VA_ARGS__),         \
                     mdv_stack_push_3(st, __VA_ARGS__),         \
                     mdv_stack_push_2(st, __VA_ARGS__),         \
                     mdv_stack_push_2(st, __VA_ARGS__),         \
                     mdv_stack_push_1(st, __VA_ARGS__),         \
                     mdv_stack_push_1(st, __VA_ARGS__))


#define mdv_stack_pop_multiple(st, count)   \
    ((st).size >= count                     \
        ? (st).data + ((st).size -= count)  \
        : 0)


#define mdv_stack_pop_one(st)               \
    ((st).size > 0                          \
        ? (st).data + --(st).size           \
        : 0)


#define mdv_stack_pop_any(st, items, M, ...) M
#define mdv_stack_pop(...)                                      \
    mdv_stack_pop_any(__VA_ARGS__,                              \
                      mdv_stack_pop_multiple(__VA_ARGS__),      \
                      mdv_stack_pop_one(__VA_ARGS__))


#define mdv_stack_empty(st)     (!(st).size)
#define mdv_stack_size(st)      (st).size
#define mdv_stack_capacity(st)  (st).capacity
#define mdv_stack_free_space(st)((st).capacity - (st).size)


#define mdv_stack_top(st)                   \
    ((st).size > 0                          \
        ? (st).data + (st).size - 1         \
        : 0)


#define mdv_stack_foreach(st, type, entry)  \
    for(type *entry = mdv_stack_top(st);    \
        entry >= (st).data;                 \
        --entry)
