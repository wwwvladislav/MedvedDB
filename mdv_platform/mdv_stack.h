#pragma once
#include "mdv_alloc.h"
#include <string.h>


#define mdv_stack(type, sz)                         \
    struct                                          \
    {                                               \
        type                             *pitem;    \
        mdv_mempool(sizeof(type) * (sz)) mempool;   \
    }


#define mdv_stack_clear(st)  \
    (st).pitem = 0;         \
    mdv_pclear((st).mempool);


#define mdv_stack_push(st, item)                                                \
    ((st).mempool.capacity - (st).mempool.size >= sizeof(item)                  \
        ? memcpy(mdv_palloc(&(st).mempool, sizeof(item)), &item, sizeof(item))  \
        : 0)


#define mdv_stack_pop(st)                                                                   \
    ((st).mempool.size >= sizeof(*(st).pitem)                                               \
        ? (st).mempool.size -= sizeof(*(st).pitem), (st).mempool.buff + (st).mempool.size   \
        : 0)


#define mdv_stack_empty(st) (!(st).mempool.size)


#define mdv_stack_top(st)                                               \
    ((st).mempool.size >= sizeof(*(st).pitem)                           \
        ? (st).mempool.buff + (st).mempool.size - sizeof(*(st).pitem)   \
        : 0)


#define mdv_stack_foreach(st, type, entry)          \
    for(type *entry = (type *)mdv_stack_top(st);    \
        (uint8_t*)entry >= (st).mempool.buff;       \
        --entry)
